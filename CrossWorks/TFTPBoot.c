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
#include "TFTPBoot.h"
#include "Wiznet/Ethernet/wizchip_conf.h"
#include "Wiznet/Ethernet/socket.h"
#include "FatFs-0-10-c/src/ff.h"
#include "Common.h"
#include "HAL/HAL.h"
#include "HAL/HALW5500.h"
#include "LuaLibs/LuaLibW5500.h"
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
//TODO Use values from flahs interface
#define FLASH_BASE             0x00400000
#define FLASH_SIZE             0x00100000
#define FLASH_PAGE_SIZE        512
#define FLASH_SECTOR_SIZE      65536
#define FLASH_PAGES_PER_SECTOR (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)
/*---------------------------------------------------------------------------*/
#define TFTP_BLOCK_SIZE 512
#define TFTP_ERROR_SIZE  80
/*---------------------------------------------------------------------------*/
#define TFTP_OPCODE_RRQ    1
#define TFTP_OPCODE_WRQ    2
#define TFTP_OPCODE_DATA   3
#define TFTP_OPCODE_ACK    4
#define TFTP_OPCODE_ERROR  5
/*---------------------------------------------------------------------------*/
#define TFTP_PORT          69
/*---------------------------------------------------------------------------*/
#define TFTP_MODE_UNKNOWN  0
#define TFTP_MODE_NETASCII 1
#define TFTP_MODE_OCTET    2
#define TFTP_MODE_MAIL     3
/*---------------------------------------------------------------------------*/
typedef struct TFTPPacket {
  uint16_t opCode;
  union {
    struct RWRQ { // RRQ, WRQ
      char fileNameAndMode[TFTP_BLOCK_SIZE];
    } rwrq;
    struct { // DATA
      uint16_t no;
      uint8_t bytes[TFTP_BLOCK_SIZE];
    } data;
    struct { // ACK
      uint16_t no;
    } ack;
    struct { // ERROR
      uint16_t code;
      char message[TFTP_ERROR_SIZE];
    } error;
  };
} __attribute__ ((packed)) TFTPPacket;
/*---------------------------------------------------------------------------*/
typedef struct TFTPBoot {
  int8_t socketNumber;
  uint32_t serverIP;
  uint16_t serverPort;
  TFTPPacket tftpPacket;
} TFTPBoot;
/*---------------------------------------------------------------------------*/
typedef struct TARHeader {
    uint8_t name[100];               //   0 
    uint8_t mode[8];                 // 100 
    uint8_t uid[8];                  // 108 
    uint8_t gid[8];                  // 116 
    uint8_t size[12];                // 124 
    uint8_t mtime[12];               // 136 
    uint8_t chksum[8];               // 148  
    uint8_t typeflag;                // 156 
    uint8_t linkname[100];           // 157 
    uint8_t magic[6];                // 257 
    uint8_t version[2];              // 263 
    uint8_t uname[32];               // 265 
    uint8_t gname[32];               // 297 
    uint8_t devmajor[8];             // 329 
    uint8_t devminor[8];             // 337 
    uint8_t prefix[155];             // 345 
 } __attribute__ ((packed)) TARHeader;
/*---------------------------------------------------------------------------*/
/* The linkflag defines the type of file */
#define  LF_OLDNORMAL '\0'       /* Normal disk file, Unix compatible */
#define  LF_NORMAL    '0'        /* Normal disk file */
#define  LF_LINK      '1'        /* Link to previously dumped file */
#define  LF_SYMLINK   '2'        /* Symbolic link */
#define  LF_CHR       '3'        /* Character special file */
#define  LF_BLK       '4'        /* Block special file */
#define  LF_DIR       '5'        /* Directory */
#define  LF_FIFO      '6'        /* FIFO special file */
#define  LF_CONTIG    '7'        /* Contiguous file */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static int16_t Transfer2(TFTPBoot *tftpBoot, uint16_t sendLength)
{
  if (tftpBoot->socketNumber < 0) 
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  int16_t result = sendto(tftpBoot->socketNumber, (uint8_t*) &tftpBoot->tftpPacket, sendLength, (uint8_t*) &tftpBoot->serverIP, tftpBoot->serverPort);
  if (result < 0)
    return result;
  int16_t timeout = 2000; // ms
  int16_t recvLength;
  for (;;) {
    recvLength = getSn_RX_RSR(tftpBoot->socketNumber);
    if (recvLength) 
      break;
    HAL_DelayMS(10);
    timeout -= 10;
    if (timeout <= 0) {
      printf("TFTP: Timeout!\n");
      return RESULT_ERROR_TIMEOUT;
    }
  }
  if (recvLength > sizeof(TFTPPacket)) 
    recvLength = sizeof(TFTPPacket);
  // Receive the packet and get server ip and port
  return recvfrom(tftpBoot->socketNumber, (uint8_t*) &tftpBoot->tftpPacket, recvLength, (uint8_t*) &tftpBoot->serverIP, &tftpBoot->serverPort); 
}
/*---------------------------------------------------------------------------*/
static int16_t Transfer(TFTPBoot *tftpBoot, uint16_t sendLength)
{
  int16_t result;
  for (uint8_t i = 0; i <= 4; i++) {
    result = Transfer2(tftpBoot, sendLength);
    if (result != RESULT_ERROR_TIMEOUT) 
      break; // For other errors then timeout a retry don't make sense
  }
  if (tftpBoot->tftpPacket.opCode == ChangeEndian16(TFTP_OPCODE_ERROR)) 
    return -(1000 + ChangeEndian16(tftpBoot->tftpPacket.error.code)); // Return the error code from server + 1000
  return result;
}
/*---------------------------------------------------------------------------*/
static int16_t FirstPacket(TFTPBoot *tftpBoot, const char *fileName)
{
  int16_t result = socket(tftpBoot->socketNumber, Sn_MR_UDP, tftpBoot->serverPort, 0x00);
  if (result < 0)
    return result;
  memset(&tftpBoot->tftpPacket, 0, sizeof(TFTPPacket));
  tftpBoot->tftpPacket.opCode = ChangeEndian16(TFTP_OPCODE_RRQ);
  strcpy(tftpBoot->tftpPacket.rwrq.fileNameAndMode, fileName);
  strcpy(&tftpBoot->tftpPacket.rwrq.fileNameAndMode[strlen(fileName) + 1], "octet"); // Request file as binary
  return Transfer(tftpBoot, 2 + TFTP_BLOCK_SIZE);
}
/*---------------------------------------------------------------------------*/
static int16_t NextPacket(TFTPBoot *tftpBoot)
{
  tftpBoot->tftpPacket.opCode = ChangeEndian16(TFTP_OPCODE_ACK); // Send the ACK
  return Transfer(tftpBoot, 2 + 2);
}
/*---------------------------------------------------------------------------*/
static int16_t ErrorPacket(TFTPBoot *tftpBoot, uint16_t code, const char *message)
{
  tftpBoot->tftpPacket.opCode = ChangeEndian16(TFTP_OPCODE_ERROR); // Send the ERROR
  tftpBoot->tftpPacket.error.code = code;
  strncpy(tftpBoot->tftpPacket.error.message, message, TFTP_ERROR_SIZE);
  sendto(tftpBoot->socketNumber, (uint8_t*) &tftpBoot->tftpPacket, 2 + 2 + TFTP_ERROR_SIZE, (uint8_t*) &tftpBoot->serverIP, tftpBoot->serverPort);
}
/*---------------------------------------------------------------------------*/
uint8_t* TFTPBoot_GetFlashImageAddress(void)
{
  uint8_t *ptr = (uint8_t*) (FLASH_BASE + FLASH_SIZE / 2);
  return ptr;
}
/*---------------------------------------------------------------------------*/
static int16_t WriteFlash(volatile uint32_t *ptrT, uint32_t *ptrS, uint16_t length)
{
  { 
    GlobalInterruptsDisable();
    uint16_t page = (uint32_t) ptrT / FLASH_PAGE_SIZE;                        // Calculate the page number from target pointer
    if ((page % FLASH_PAGES_PER_SECTOR) == (FLASH_PAGES_PER_SECTOR - 1))      // If page is the last page in sector then
      IAP(ES, page + FLASH_PAGES_PER_SECTOR);                                 //   erase the next sector
    for (uint8_t i = 0; i < length / 4; i++)                                  // Copy to flash write buffer
      ptrT[i] = ptrS[i];
    IAP(WP, page);                                                            // Do the write
    GlobalInterruptsEnable();
  }
  // Verify FLASH page
  {
    for (uint8_t i = 0; i < length / 4; i++) {                                // Verify all long words in the page
      if (*ptrT != *ptrS) {
        DEBUG("TFTP: Flash write fail at 0x%08x: 0x%08X ~ 0x%08X\n", ptrT, *ptrT, *ptrS);
        return FATAL_FLASH_WRITE;
      }
      ptrT++;
      ptrS++;
    }
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int32_t TFTPBoot_Flash(int8_t socketNumber)
{
  TFTPBoot tftpBoot;
  tftpBoot.socketNumber = socketNumber;
  tftpBoot.serverIP = IP_BROADCAST;
  tftpBoot.serverPort = TFTP_PORT;

  int16_t Do(void)
  {
    int16_t result = FirstPacket(&tftpBoot, "FlashDisk.fat");
    if (result < 0) 
      return result;
    if (result != sizeof(TFTPPacket)) 
      return RESULT_ERROR_BAD_SIZE;
    uint32_t *ptrT = (uint32_t*) TFTPBoot_GetFlashImageAddress();            // Get the address to store the archive
    puts("TFTP: Write 'FlashDisk.fat'");
    uint16_t page = (uint32_t) ptrT / FLASH_PAGE_SIZE;                       // Calculate the page number from target pointer
    IAP(ES, page);                                                           // Erase this sector
    uint32_t count = 0;
    for (;;) {
      count += result - 4;                                                   // Count the bytes
      result = WriteFlash(ptrT, (uint32_t*) tftpBoot.tftpPacket.data.bytes, FLASH_PAGE_SIZE);
      if (result < 0) {
        // Erase first sector to prevent currupted filesystem
        uint32_t *ptrT = (uint32_t*) TFTPBoot_GetFlashImageAddress();        // Get the address to store the archive
        uint16_t page = (uint32_t) ptrT / FLASH_PAGE_SIZE;                   // Calculate the page number from target pointer
        IAP(ES, page);                                                       // Erase this sector
        ErrorPacket(&tftpBoot, 0, "FLASH write error");
        return result;
      }
      ptrT += FLASH_PAGE_SIZE / 4;
      result = NextPacket(&tftpBoot);
      if (result < 0) 
        return result;
      if (result < sizeof(TFTPPacket)) 
        break; // If last packet exit the loop
    }
    return count;
  }

  int16_t result = Do();
  close(socketNumber);
  return result;
}
/*---------------------------------------------------------------------------*/
int32_t TFTPBoot_SDCard(int8_t socketNumber, const char drive)
{
  TFTPBoot tftpBoot;
  tftpBoot.socketNumber = socketNumber;
  tftpBoot.serverIP = IP_BROADCAST;
  tftpBoot.serverPort = TFTP_PORT;
  FIL file;
  bool_t fileIsOpen = false; 

  int16_t Do(void)
  {
    int16_t result = FirstPacket(&tftpBoot, "SDCardImage.tar");
    if (result < 0) 
      return result;
    if (result != sizeof(TFTPPacket)) 
      return RESULT_ERROR_BAD_SIZE;

    for (;;) {
      TARHeader *tarHeader = (TARHeader*) tftpBoot.tftpPacket.data.bytes; 
      if (tarHeader->typeflag == LF_NORMAL) {

        if (fileIsOpen) {
          f_close(&file);
          fileIsOpen = false;
        }
        {
          char path[128] = {drive, ':', 0};
          strcat(path, tarHeader->name);
          f_unlink(path); // Delete the file
          FRESULT result = f_open(&file, path, FA_WRITE | FA_OPEN_ALWAYS);
          if (result != FR_OK) {
            ErrorPacket(&tftpBoot, 0, "Can't open file");
            return RESULT_ERROR;
          }
          fileIsOpen = true;
        }

        uint32_t size = 0;
        for (uint8_t i = 0; i <= 10; i++) 
          size = (size << 3) + (tarHeader->size[i] - '0');
        printf("TFTP: Write file %c:%s, %lu bytes\n", drive, tarHeader->name, size);

        uint32_t n = (size + 511) / 512;
        for (uint32_t i = 0; i < n; i++) { 
          // Read next packet
          result = NextPacket(&tftpBoot);
          if (result < 0) 
            return result;
          if (result < sizeof(TFTPPacket)) 
            return RESULT_ERROR;
          UINT byteToWrite = size > 512 ? 512 : size;
          UINT bytesWritten;
          FRESULT result = f_write(&file, tftpBoot.tftpPacket.data.bytes, byteToWrite, &bytesWritten); 
          if ((result != FR_OK) || (byteToWrite != bytesWritten)) {
            ErrorPacket(&tftpBoot, 0, "Can't write file");
            return RESULT_ERROR;
          }
          size -= bytesWritten;
        }

      } else if (tarHeader->typeflag == LF_DIR) {

        printf("TFTP: Create directory %c:%s\n", drive, tarHeader->name);
        if (tarHeader->name[1]) {
          char path[128] = {drive, ':', 0};
          strcat(path, &tarHeader->name[1]);
          path[strlen(path) - 1] = 0;
          FRESULT result = f_mkdir(path);
          if ((result != FR_OK) && (result != FR_EXIST)) {
            ErrorPacket(&tftpBoot, 0, "Can't create directory");
            return RESULT_ERROR;
          }
        }

      } else {

        if (!tarHeader->typeflag && !tarHeader->magic[0]) 
          return RESULT_OK;
        ErrorPacket(&tftpBoot, 0, "Bad TAR file");
        return RESULT_ERROR;

      }
      result = NextPacket(&tftpBoot);
      if (result < 0) 
        return result;
      if (result < sizeof(TFTPPacket)) 
        break; // If last packet exit the loop
    }
    return RESULT_OK;
  }

  int16_t result = Do();
  close(socketNumber);
  if (fileIsOpen) 
    f_close(&file);

  return result;
}
/*---------------------------------------------------------------------------*/
const char* TFTPBoot_ErrorToString(int16_t result)
{
  switch (result) {
    case -1001 : return "File not found"; 
    case -1002 : return "Access permitted"; 
    case -1003 : return "Disk full or allocation exceeded";
    case -1004 : return "Illegal packet operation"; 
    case -1005 : return "Unknown transfer ID";
    case -1006 : return "File already exists";
    case -1007 : return "No such user"; 
  }
  return HAL_ErrorToString(result);
}
/*---------------------------------------------------------------------------*/
