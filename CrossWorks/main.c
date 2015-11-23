/*
 *
 *
 *  Copyright (C) Nogs GmbH, Andre Riesberg
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, 
 *  write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
/*---------------------------------------------------------------------------*/
#include "C:\Nogs\01_Development\Nodes\Software\Librarys\Nogs.h"
#include "Lint.h"
#include "SAM_Series.h"
#include "HAL/HAL.h"
#include "HAL/HALW5500.h"
#include "HAL/HALHSMCI.h"
#include "WizNet/ZeroConf.h"
#include "FatFs-0-10-c/src/ff.h"
#include "FatFsWrapper.h"
#include "Memory.h"
#include "cn-2.0.0/src/CN.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "SysLog.h"
#ifdef HAS_ATSHA204
  #include "ATSHA204.h"
#endif
#if defined(TFTP_BOOT_FLASH) || defined(TFTP_BOOT_SD)
  #include "TFTPBoot.h"
#endif
#ifdef USEFFS
  #include "FFS.h"
#endif
#include "LuaLibs/LuaLibSys.h"
#include "LuaLibs/LuaLibBASE64.h"
#include "LuaLibs/LuaLibW5500.h"
#include "LuaLibs/LuaLibPin.h"
#include "LuaLibs/LuaLibSerial.h"
#include "LuaLibs/LuaLibJSON.h"
#ifdef HAS_I2C
  #include "LuaLibs/LuaLibI2C.h"
#endif
#ifdef HAS_WS2812
  #include "LuaLibs/LuaLibWS2812.h"
#endif
#ifdef HAS_LIS
  #include "LuaLibs/LuaLibLIS.h"
#endif
#ifdef HAS_EVE
  #include "LuaLibs/LuaLibEVE.h"
#endif
#ifdef HAS_SOFT_PWM
  #include "LuaLibs/LuaLibSoftPWM.h"
#endif
#ifdef HAS_ADC
  #include "LuaLibs/LuaLibADC.h"
#endif
#ifdef HAS_SOFT_COUNTER
  #include "LuaLibs/LuaLibSoftCounter.h"
#endif
/*---------------------------------------------------------------------------*/
#undef DEBUG
#ifdef STARTUP_FROM_RESET
  #define DEBUG(...)
#else
  #if 1
    extern int debug_printf(const char *format, ...);
    #define DEBUG debug_printf
  #else
    #define DEBUG(...)
  #endif
#endif
/*---------------------------------------------------------------------------*/
lua_State *L;
FATFS fileSystem[2];
/*---------------------------------------------------------------------------*/
void __assert(int ok)
{
  if (!ok) 
    DEBUG("**lua_assert**");
}
/*---------------------------------------------------------------------------*/
#if _USE_LFN == 3
  void* ff_memalloc(size_t size)
  {
    return Memory_Allocate(size, 0);
  }
#endif
/*---------------------------------------------------------------------------*/
#if _USE_LFN == 3
  void ff_memfree(void *mem)
  {
    Memory_Free(mem, 0);
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  // If watchdog is enabled DumpDirectory can take too match time, so 
  // watchdog is automatic disabled by watchdog disable timer!
  #define DumpDirectory(...)
#else
  void DumpDirectory(const char *path, const char *title)
  {
    //return;
    uint32_t fre_sect = 0;
    FATFS *fs;
    DWORD fre_clust;
    if (f_getfree(path, &fre_clust, &fs) == FR_OK) 
      fre_sect = fre_clust * fs->csize;
    DEBUG("\n  Directory of %s (%s)\n", path, title);
    DIR dir;
    int result = f_opendir(&dir, path);
    if (result == FR_OK) {
      uint16_t fileCount = 0, dirCount = 0;
      FILINFO fno;
      fno.lfname = Memory_Allocate(_MAX_LFN + 1, 0);
      fno.lfsize = _MAX_LFN + 1;
      for (;;) {
        result = f_readdir(&dir, &fno);
        if ((result != FR_OK) || (fno.fname[0] == 0) || (fno.fname[0] == 0xFF))
          break;
        if (fno.fattrib & AM_DIR) {
          DEBUG("                           <DIR>       %s\n", fno.lfname[0] ? fno.lfname : fno.fname);
          dirCount++;
        } else {
          DEBUG(
            "    %u/%02u/%02u %02u:%02u %c%c%c%c %12u %s\n",
            (fno.fdate >> 9) + 1980,
            fno.fdate >> 5 & 15,
            fno.fdate & 31,
            fno.ftime >> 11,
            fno.ftime >> 5 & 63,
            (fno.fattrib & AM_RDO) ? 'R' : '-',
            (fno.fattrib & AM_HID) ? 'H' : '-',
            (fno.fattrib & AM_SYS) ? 'S' : '-',
            (fno.fattrib & AM_ARC) ? 'A' : '-',
            fno.fsize,
            fno.lfname[0] ? fno.lfname : fno.fname
          );
          fileCount++;
        }
      }
      Memory_Free(fno.lfname, _MAX_LFN + 1);
      DEBUG("  Files: %u, directorys %u, free %u kB\n\n", fileCount, dirCount, fre_sect / 2);
    } else
      DEBUG("Error %u\n", result);
  }
#endif
/*---------------------------------------------------------------------------*/
bool FileExits(const char *path)
{
  FILE *f = fopen(path, "r");
  if (!f) 
    return false;
  fclose(f);
  return true;
}
/*---------------------------------------------------------------------------*/
void LuaLibLevelSys_Register(lua_State *L)
{
  LuaLibSys_Register(L);
  LuaLibBASE64_Register(L);
  LuaLibW5500_Register(L);
  LuaLibPin_Register(L);
  LuaLibSerial_Register(L);
  LuaLibJSON_Register(L);
  #ifdef HAS_I2C
    LuaLibI2C_Register(L);
  #endif
  #ifdef HAS_WS2812
    LuaLibWS2812_Register(L);
  #endif
  #ifdef HAS_LIS
    LuaLibLIS_Register(L);
  #endif
  #ifdef HAS_EVE
    LuaLibEVE_Register(L);
  #endif
  #ifdef HAS_SOFT_PWM
    LuaLibSoftPWM_Register(L);
  #endif
  #ifdef HAS_ADC
    LuaLibADC_Register(L);
  #endif
  #ifdef HAS_SOFT_COUNTER
    LuaLibSoftCounter_Register(L);
  #endif
}
/*---------------------------------------------------------------------------*/
#define STATUS_NETWORK_ACTIVE   BIT(0)
#define STATUS_SDCARD_FS_ACTIVE BIT(1)
#define STATUS_FLASH_FS_ACTIVE  BIT(2)
#define STATUS_FFS_FS_ACTIVE    BIT(3)
#define STATUS_SDCARD_STARTUP   BIT(4)
#define STATUS_FLASH_STARTUP    BIT(5)
#define STATUS_FFS_STARTUP      BIT(6)
/*---------------------------------------------------------------------------*/
int main(void)
{
  DEBUG("main\n");
  uint8_t status = 0;
  memset(fileSystem, 0, sizeof(fileSystem));
  //
  // Startup mandatory objects and show the NOGS banner
  //
  SystemInit();
  HAL_Init();
  Memory_Init();
  puts(NOGS_BANNER);
  puts(TOSTRING(NOGS_DEVICE) " " TOSTRING(NOGS_TYPE) " " __DATE__ " " __TIME__ " Made by AR.\n");
  //printf("System core clock %u MHz\n", SystemCoreClock / 1000000L);

  //
  // Init the crypto chip
  //
  #ifdef HAS_ATSHA204
    uint8_t serialNumber[9];
    {
      ATSHA204_Init();
      if (!ATSHA204_GetSerialNumber(serialNumber))
        HAL_SysFatal(FATAL_NO_CRYPTO_CHIP);
        // NOTE: If this fails CPU clock could be wrong!!! See the notes in Common.h
      DEBUG("SHA204: Serial number ");
      for (uint8_t i = 0; i <= 8; i++) 
        DEBUG("%02X", serialNumber[i]);
      DEBUG("\n");
    }
  #endif

  //
  // Init the network. Depending on the configuration
  // * Try to get an ip address via ZeroConf protocol
  // or
  // * Use the default IP address 192.168.1.249 and MAC FEED01000000
  //
  LuaLibW5500_Init();
  uint8_t mac[6] = {0xFE, 0xED, 0x01, 0, 0, 0};
  uint32_t ip, mask;
  {
    #ifdef NET_ZEROCONF
      {
        HAL_SysLED(LED_WAIT_FOR_LINK);
        #ifdef HAS_ATSHA204
          memcpy(&mac[3], &serialNumber[2], 3);
        #endif
        mask = IP_MAKE(255, 255, 0, 0);
        if (!HALW5500_Init(mac)) HAL_SysFatal(FATAL_ETHERNT_FAIL);
        HALW5500_SetIP(IP_NULL, mask);
        if (HALW5500_WaitForLinkUp(3)) {
          HAL_SysLED(LED_ZEROCONF);
          int8_t result = ZeroConfig(mac, &ip);
          if (result < 0) 
            HAL_SysFatal(FATAL_ZERO_CONFIG_FAIL);
          HALW5500_SetIP(ip, mask);
          HAL_SysLED(LED_WAIT_FOR_LINK);
          if (HALW5500_WaitForLinkUp(3))
            status |= STATUS_NETWORK_ACTIVE;
        }
      }
    #else
      HAL_SysLED(LED_WAIT_FOR_LINK);
      if (!HALW5500_Init(mac))
        HAL_SysFatal(FATAL_ETHERNT_FAIL);
      ip = IP_MAKE(192, 168, 1, 249);
      mask = IP_MAKE(255, 255, 255, 0);
      HALW5500_ConfigIP(ip, mask);
      if (HALW5500_WaitForLinkUp(3))
        status |= STATUS_NETWORK_ACTIVE;
    #endif
  }

  //
  // If network is active send a syslog startup message to anounce the boot of this device.
  // If TFTP boot is requested, try to read the fs image and store it into the main
  // flash memory.
  //
  HAL_SysLED(LED_RUNNING);

  {
    int8_t socketNumber = LuaLibW5500_GetFreeSocket();
    Memory_StartTracker(socketNumber, IP_BROADCAST);
    DEBUG("Start memory tracker %d\n", socketNumber);
  }

  if (status & STATUS_NETWORK_ACTIVE) {
    {
      char ipString[16];
      IPToString(ipString, sizeof(ipString), ip);
      char maskString[16];
      IPToString(maskString, sizeof(maskString), mask);
      char macString[13];
      MACToSTring(macString, sizeof(macString), mac);
      char s[256];
      snprintf(s, sizeof(s), "[{\"startup\":\"" TOSTRING(NOGS_DEVICE) " " TOSTRING(NOGS_TYPE) " " __DATE__ " " __TIME__ "\"},{\"type\":\"" TOSTRING(NOGS_TYPE) "\"},{\"ip\":\"%s\"},{\"mask\":\"%s\"},{\"mac\":\"%s\"}]", ipString, maskString, macString);
      int8_t socketNumber = LuaLibW5500_GetFreeSocket();
      SysLog_Init(socketNumber);
      SysLog_Config(IP_BROADCAST, SYSLOG_PORT);
      SysLog_Send(SYSLOG_KERN | SYSLOG_NOTICE, s);
      SysLog_Config(IP_BROADCAST, SYSLOG_NOGS_PORT);
      SysLog_Send(SYSLOG_KERN | SYSLOG_NOTICE, s);
    }

    #ifdef REDIRECT_STDOUT_TO_SYSLOG
      printf(NOGS_BANNER);
      puts(" " TOSTRING(NOGS_TYPE) " " __DATE__ " " __TIME__ " Made by AR.\n\n");
    #endif

    #ifdef TFTP_BOOT_FLASH
      {
        HAL_SysLED(1);
        puts("TFTP: Try to get 'FlashDisk.fat'");
        int16_t socketNumber = LuaLibW5500_GetFreeSocket();
        int32_t result = TFTPBoot_Flash(socketNumber);
        if (result < 0)
          printf("TFTP: %s\n", TFTPBoot_ErrorToString(result));
        else if (result > 0)
          printf("TFTP: Done, receiving %lu bytes\n", result);
        LuaLibW5500_FreeSocket(socketNumber);
        HAL_SysLED(0b10);
      }
    #endif
    #ifdef USEFFS
      {
        int8_t socketNumber = LuaLibW5500_GetFreeSocket();
        int16_t result = FFS_Init(socketNumber, IP_BROADCAST);
        if (result < 0)
          LuaLibW5500_FreeSocket(socketNumber);
        else {
          DumpDirectory("2:/", "FFS");
          status |= STATUS_FFS_FS_ACTIVE;
          if (FileExits("2:/Startup.lua"))
            status |= STATUS_FFS_STARTUP;
        }
      }
    #endif
  }

  //
  // Check for SD-Card and try to mount the card.
  // If mount is succesfull show the root directory contens.
  //
  {
    SDCardState sdCardState;
    do {
      sdCardState = HAL_GetSDCardState();
    } while (sdCardState == csUnknown);
    if (sdCardState & SDCARD_STATE_CARD_MASK) {
      DEBUG("SD: Card found, init HSMCI... ");
      if (HALHSMCI_Init() < 0)
        DEBUG("fail\n");
      else {
        DEBUG("done!\n");
        DEBUG("SD: Try to mount filesystem... ");
        FRESULT result = f_mount(&fileSystem[1], "1:/", 1);
        if (result == FR_OK) {
          DEBUG("done!\n");
          puts("SD: Card found and mounted");
          #ifdef TFTP_BOOT_SD
            HAL_SysLED(1);
            puts("TFTP: Try to get 'SDCardImage.tar'");
            int16_t socketNumber = LuaLibW5500_GetFreeSocket();
            int32_t result = TFTPBoot_SDCard(socketNumber, '1');
            if (result < 0)
              printf("TFTP: %s\n", TFTPBoot_ErrorToString(result));
            else if (result > 0)
              printf("TFTP: Done, receiving %lu bytes\n", result);
            LuaLibW5500_FreeSocket(socketNumber);
            HAL_SysLED(0b10);
          #endif
          DumpDirectory("1:/", "SD");
          status |= STATUS_SDCARD_FS_ACTIVE;
          if (FileExits("1:/Startup.lua"))
            status |= STATUS_SDCARD_STARTUP;
        } else {
          DEBUG("error %u\n", result);
          #if 0
            DEBUG("SD: Formatting... ");
            result = f_mkfs("1:/", 0, 0);
            if (result == FR_OK)
              DEBUG("done!\n");
            else
              DEBUG("error %u\n", result);
          #endif
        }
      }
    } else
      DEBUG("SD: No card found\n");
  }

  //
  // Try to mount the flash fs
  // If mount is succesfull show the root directory contens.
  //
  {
    DEBUG("FLASHFS: Try to mount flash filesystem... ");
    FRESULT result = f_mount(&fileSystem[0], "0:/", 1);
    if (result == FR_OK) {
      DEBUG("done!\n");
      DumpDirectory("0:/", "FLASH");
      status |= STATUS_FLASH_FS_ACTIVE;
      if (FileExits("0:/Startup.lua"))
        status |= STATUS_FLASH_STARTUP;
    } else 
      DEBUG("error %u\n", result);
  }

  //
  // Drive information
  //
  void DriveInformationTitle(void)
  {
    puts("");
    puts("| Drive | Name  | Description                                 | Access | Startup.lua | Boot |");
    puts("+-------+-------+---------------------------------------------+--------+-------------+------+");
  }

  void DriveInformationRow(uint8_t drive, const char *name, const char *info, const char *rw, bool hasStartupLua, bool boot)
  {
    printf("| %u:/   | %-6s| %-44s| %-7s| %-12s| %-5s|\n", drive, name, info, rw, hasStartupLua ? "yes" : "no", boot ? "yes" : "no"); 
  }

  uint8_t bootDrive = 0;
  if (status & STATUS_SDCARD_STARTUP)
    bootDrive = 1;
  if (status & STATUS_FFS_STARTUP)
    bootDrive = 2;

  DriveInformationTitle();
  if (status & STATUS_FLASH_FS_ACTIVE) 
    DriveInformationRow(0, "FLASH", "Filesystem inside CPU (flash memory), local", "r", status & STATUS_FLASH_STARTUP, bootDrive == 0);
  if (status & STATUS_SDCARD_FS_ACTIVE) 
    DriveInformationRow(1, "SD", "SD-Card, local", "rw", status & STATUS_SDCARD_STARTUP, bootDrive == 1);
  if (status & STATUS_FFS_FS_ACTIVE) 
    DriveInformationRow(2, "FFS", "Far File System, remote (Nogs-IDE)", "rw", status & STATUS_FFS_STARTUP, bootDrive == 2);
  puts("");

  //
  // Try to start Startup.lua from SD-Card or Flash fs.
  //
  {
    if (!(status & (STATUS_FLASH_FS_ACTIVE | STATUS_SDCARD_FS_ACTIVE | STATUS_FFS_FS_ACTIVE)))
      HAL_SysFatal(FATAL_NO_DRIVE_FOUND);
    if (!(status & (STATUS_FLASH_STARTUP | STATUS_SDCARD_STARTUP | STATUS_FFS_STARTUP)))
      HAL_SysFatal(FATAL_NO_STARTUP);
    char path[15];
    strcpy(path, "0:/Startup.lua");
    path[0] += bootDrive;
    CN_Init();
    DEBUG("CN: Run\n");
    CN_Run(path);
  }

  //
  // This should not be happen. Lua must run all the time!
  //
  HAL_SysFatal(FATAL_MAIN_LOOP_ENDS);
}
/*---------------------------------------------------------------------------*/
