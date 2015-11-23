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
#include "FFS.h"
#include "Common.h"
#include "HAL/HAL.h"
#include "Wiznet/Ethernet/wizchip_conf.h"
#include "Wiznet/Ethernet/socket.h"
#include "FatFs-0-10-c/src/ff.h"
#include "FatFsWrapper.h"
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
#define FFS_RECV_PORT 51121
#define FFS_SEND_PORT 51122
/*---------------------------------------------------------------------------*/
#define COMMAND_INI 1000
/*---------------------------------------------------------------------------*/
typedef struct FFSPacket {
  uint32_t command;
  int32_t parameter;
  uint8_t data[512];
} __attribute__ ((packed)) FFSPacket;
/*---------------------------------------------------------------------------*/
struct {
  int8_t socketNumber;
  uint32_t ip;
  FATFS fileSystem;
} ffs;
/*---------------------------------------------------------------------------*/
static int16_t Transfer2(FFSPacket *ffsPacket)
{
  if (ffs.socketNumber < 0) 
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  int16_t result = sendto(ffs.socketNumber, (uint8_t*) ffsPacket, 8, (uint8_t*) &ffs.ip, FFS_SEND_PORT); // Send th packet
  if (result < 0)
    return result;
  int16_t timeout = 500; // ms
  int16_t recvLength;
  for (;;) {
    recvLength = getSn_RX_RSR(ffs.socketNumber);
    if (recvLength) 
      break;
    HAL_DelayMS(10);
    timeout -= 10;
    if (timeout <= 0)
      return RESULT_ERROR_TIMEOUT;
  }
  uint32_t ip;
  uint16_t port;
  if (recvLength > sizeof(FFSPacket)) 
    recvLength > sizeof(FFSPacket);
  return recvfrom(ffs.socketNumber, (uint8_t*) ffsPacket, recvLength, (uint8_t*) &ip, &port); // Receive the packet
}
/*---------------------------------------------------------------------------*/
static int16_t Transfer(FFSPacket *ffsPacket)
{
  int16_t result;
  for (uint8_t i = 0; i <= 4; i++) {
    result = Transfer2(ffsPacket);
    if (result != RESULT_ERROR_TIMEOUT) 
      break; // For other errors then timeout a retry don't make sense
  }
  if (ffsPacket->parameter < 0) 
    return ffsPacket->parameter; // Return the error code from server
  return result;
}
/*---------------------------------------------------------------------------*/
int16_t FFS_Init(int8_t socketNumber, uint32_t ip)
{
  DEBUG("FFS: Init ");
  memset(&ffs, 0, sizeof(ffs));
  ffs.socketNumber = socket(socketNumber, Sn_MR_UDP, FFS_RECV_PORT, 0x00);
  ffs.ip = ip;
  FFSPacket ffsPacket;
  ffsPacket.command = COMMAND_INI;
  int16_t result = Transfer(&ffsPacket);
  if (result >= 8) {
    if (ffsPacket.parameter > 0) {
      DEBUG("Drive has %u blocks, try to mount... ", ffsPacket.parameter);
      FRESULT result = f_mount(&ffs.fileSystem, "2:/", 1);
      if (result == FR_OK) {
        DEBUG("success!\n");
        return RESULT_OK;
      }
    }
  }
  DEBUG("fail!\n");
  return result < 0 ? result : RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
bool FFS_GetState()
{
  return ffs.socketNumber > 0;
}
/*---------------------------------------------------------------------------*/
int16_t FFS_ReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead)
{
  FFSPacket ffsPacket;
  for (int i = 0; i < blocksToRead; i++) {
    ffsPacket.command = 1001;
    ffsPacket.parameter = lba++;
    int16_t result = Transfer(&ffsPacket);
    if (result < 0) 
      return result;
    memcpy(buffer, ffsPacket.data, 512);
    buffer += 512;
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t FFS_WriteBlocks(const void *buffer, uint32_t lba, uint32_t blocksToWrite)
{
  FFSPacket ffsPacket;
  for (int i = 0; i < blocksToWrite; i++) {
    ffsPacket.command = 1002;
    ffsPacket.parameter = lba++;
    memcpy(ffsPacket.data, buffer, 512);
    int16_t result = Transfer(&ffsPacket);
    if (result < 0) 
      return result;
    buffer += 512;
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
