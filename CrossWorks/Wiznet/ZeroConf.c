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
#include "ZeroConf.h"
#include <stdlib.h>
#include "../HAL/HAL.h"
#include "../HAL/HALW5500.h"
#include "../Wiznet/Ethernet/wizchip_conf.h"
#include "../Wiznet/Ethernet/socket.h"
#include "../LuaLibs/LuaLibW5500.h"
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
/*---------------------------------------------------------------------------
  Zeroconf
  ---------------------------------------------------------------------------*/
#ifdef ZEROCONFIG_FAST
  #define DELAY_MIN_MS   250
  #define DELAY_RANGE_MS 250
#else
  #define DELAY_MIN_MS   1000
  #define DELAY_RANGE_MS 1000
#endif
/*---------------------------------------------------------------------------*/
#define ZEROCONFIG_IP_USED 1
/*---------------------------------------------------------------------------*/
typedef struct ETH {
  uint8_t destMAC[6];                 // MAC of target device
  uint8_t sourceMAC[6];               // MAC of source device
  uint16_t type;                      // IPv4 == 0x800
} __attribute__ ((packed)) ETH;       // = 14 Byte
/*---------------------------------------------------------------------------*/
typedef struct ARP {
  uint16_t type;                      // Network protocol type. Ethernet == 1
  uint16_t protocolType;              // Internetwork protocol type. IPv4 == 0x0800
  uint8_t size;                       // Length in byte of a hardware address. Here 6
  uint8_t protocolSize;               // Length in byte of addresses used for upper layer protocol. Here 4
  uint16_t opCode;                    // Operation that the sender is performing: 1 = request, 2 = reply
  uint8_t sourceMAC[6];               // MAC of source device
  uint32_t sourceIP;                  // IP address of source device
  uint8_t destMAC[6];                 // MAC of target device
  uint32_t destIP;                    // IP address of target device
} __attribute__ ((packed)) ARP;
/*---------------------------------------------------------------------------*/
typedef struct ARPPacket {
  ETH eth;
  ARP arp;
} __attribute__ ((packed)) ARPPacket;
/*---------------------------------------------------------------------------*/
/*!
 * \brief Generate a pseudo random number form [ 0 - range )
 */
static uint16_t random(uint16_t range)
{
  return (uint32_t) rand() * (uint32_t) range / RAND_MAX;
}
/*---------------------------------------------------------------------------*/
static int8_t DoARPProbe(uint8_t* thisMAC, uint32_t probeIP, int16_t timeoutMS)
{
  ARPPacket arpProbe;
  memset(&arpProbe, 0, sizeof(arpProbe));

  memset(arpProbe.eth.destMAC, 0xFF, 6); // Broadcast
  memcpy(arpProbe.eth.sourceMAC, thisMAC, 6);
  arpProbe.eth.type = ChangeEndian16(0x0806); // ARP

  arpProbe.arp.type = ChangeEndian16(1);
  arpProbe.arp.protocolType = ChangeEndian16(0x0800);
  arpProbe.arp.size = 6;
  arpProbe.arp.protocolSize = 4;
  arpProbe.arp.opCode = ChangeEndian16(1);
  memcpy(arpProbe.arp.sourceMAC, thisMAC, 6);
  memset(arpProbe.arp.destMAC, 0xFF, 6); // Broadcast
  arpProbe.arp.destIP = probeIP;

  uint32_t dummyIP = -1;
  if (sendto(0, (uint8_t*) &arpProbe, sizeof(arpProbe), (uint8_t*) &dummyIP, -1) == 0) 
    return RESULT_ZEROCONFIG_ERROR_CANT_SEND_ARP_PROBE;
  while (timeoutMS > 0) {
    for (;;) {
      int16_t recvLength = getSn_RX_RSR(0);
      if (recvLength <= 0) 
        break;
      assert(recvLength < 2048);
      uint8_t recv[2048]; // recvLength can't be greater than MTU, so size is equal to W5500 buffer size
      uint16_t dummyPort;
      uint32_t dummyIP;
      memset(recv, 0, sizeof(recv));
      int16_t recvLength2 = recvfrom(0, (uint8_t*) recv, recvLength, (uint8_t*) &dummyIP, &dummyPort);
      if (recvLength2 == SOCKFATAL_PACKLEN) {
        //?? Why can this happen?
        //DEBUG("\nReceive %d, result %d\n", recvLength, recvLength2);
        //DEBUG("\n--%d: %02X%02X%02X%02X%02X%02X", recvLength, recv[0], recv[1], recv[2], recv[3], recv[4], recv[5]);
        //DEBUG(" %02X%02X%02X%02X%02X%02X\n", recv[6], recv[7], recv[8], recv[9], recv[10], recv[11]);
        socket(0, Sn_MR_MACRAW, 0, Sn_MR_MFEN);
        break;
      } else if (recvLength2 < 0) {

      } else if (memcmp(recv, thisMAC, 6) == 0) {
        ARPPacket *arpReplay = recv;
        // Is this the replay to my arp probe?
        if ((arpReplay->arp.type == ChangeEndian16(0x0806)) && (memcmp(arpReplay->arp.sourceMAC, thisMAC, 6) == 0)) {
          DEBUG(" Device found with MAC %02X%02X%02X%02X%02X%02X\n", recv[6], recv[7], recv[8], recv[9], recv[10], recv[11]);
          return ZEROCONFIG_IP_USED;
        }
      }
    }
    HAL_DelayMS(10);
    timeoutMS -= 10;
  }
  return RESULT_ZEROCONFIG_IP_FREE;
}
/*---------------------------------------------------------------------------*/
static int8_t ZeroConfig2(uint8_t *thisMAC, uint32_t *freeIP)
{
  uint32_t seed = 0;
  memcpy(&seed, &thisMAC[3], 3);
  srand(seed);
  if (socket(0, Sn_MR_MACRAW, 0, Sn_MR_MFEN) != 0) 
    return RESULT_ZEROCONFIG_ERROR_CANT_OPEN_SOCKET;
  HAL_DelayMS(DELAY_MIN_MS + random(DELAY_RANGE_MS));
  for (uint8_t i = 0; i < 20; i++) {
    uint32_t probeIP = IP_MAKE(169, 254, random(254) + 1, random(254) + 1);
    DEBUG("ZC: ARP probe 169.254.%u.%u", (probeIP >> 16) & 0xFF, probeIP >> 24);
    int8_t result;
    for (uint8_t i = 0; i <= 2; i++) {
      DEBUG(".");
      result = DoARPProbe(thisMAC, probeIP, i == 2 ? DELAY_MIN_MS + DELAY_RANGE_MS : DELAY_MIN_MS + random(DELAY_RANGE_MS));
      if (result < 0) 
        return result;
      if (result == ZEROCONFIG_IP_USED) 
        break;
    }
    if (result == RESULT_ZEROCONFIG_IP_FREE) {
      DEBUG(" free\n");
      *freeIP = probeIP;
      return RESULT_ZEROCONFIG_IP_FREE;
    }
    DEBUG(" in use\n");
  }
  DEBUG(" give up after 20 trys\n");
  return RESULT_ZEROCONFIG_ERROR_GIVE_UP;
}
/*---------------------------------------------------------------------------*/
int16_t ZeroConfig(uint8_t *thisMAC, uint32_t *freeIP)
{
  if (LuaLibW5500_GetSocket0() < 0) 
    return RESULT_ZEROCONFIG_ERROR_SOCKET0_IN_USE;
  int8_t result = ZeroConfig2(thisMAC, freeIP);
  LuaLibW5500_FreeSocket(0);
  return result;
}
/*---------------------------------------------------------------------------*/
