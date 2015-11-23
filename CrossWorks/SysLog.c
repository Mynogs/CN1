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
#include "SysLog.h"
#include "../Wiznet/Ethernet/wizchip_conf.h"
#include "../Wiznet/Ethernet/socket.h"
#include "../Common.h"
#include "../Memory.h"
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
#define STATIC_BUFFER
/*---------------------------------------------------------------------------*/
struct {
  int8_t socketNumber;
  uint32_t ip;
  uint16_t sendPort;
  #ifdef STATIC_BUFFER
    char buffer[256];
  #endif
} sysLog;
/*---------------------------------------------------------------------------*/
void SysLog_Init(int8_t socketNumber)
{
  memset(&sysLog, 0, sizeof(sysLog));
  sysLog.socketNumber = socketNumber;
}
/*---------------------------------------------------------------------------*/
void SysLog_Config(uint32_t ip, uint16_t sendPort)
{
  sysLog.ip = ip;
  sysLog.sendPort = sendPort;
  sysLog.socketNumber = socket(sysLog.socketNumber, Sn_MR_UDP, 0, 0x00);
}
/*---------------------------------------------------------------------------*/
void SysLog_Send(uint16_t priority, const char *message)
{
  #ifdef STATIC_BUFFER
    if ((sysLog.socketNumber >= 0) && sysLog.ip)
      snprintf(sysLog.buffer, sizeof(sysLog.buffer), "<%u>Jan  1 00:00:00 %s", priority, message);
    sendto(sysLog.socketNumber, sysLog.buffer, strlen(sysLog.buffer), (uint8_t*) &sysLog.ip, sysLog.sendPort);
  #else
    if ((sysLog.socketNumber >= 0) && sysLog.ip) {
      size_t size = 1 + 5 + 17 + strlen(message) + 1;
      char *s = Memory_Allocate(size, 0);
      if (s) {
        snprintf(s, size, "<%u>Jan  1 00:00:00 %s", priority, message);
        sendto(sysLog.socketNumber, s, strlen(s), (uint8_t*) &sysLog.ip, sysLog.sendPort);
        Memory_Free(s, size);
      }
    }
  #endif
}
/*---------------------------------------------------------------------------*/











