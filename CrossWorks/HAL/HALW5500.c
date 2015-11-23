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
#include "../Wiznet/Ethernet/wizchip_conf.h"
#include "../LuaLibs/LuaLibW5500.h"
#include "HALW5500.h"
#include "HAL.h"
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
wiz_NetInfo netInfo;
/*---------------------------------------------------------------------------*/
static uint8_t HALW5500_Read()
{
  return HAL_SPIEthWriteRead(0);
}
/*---------------------------------------------------------------------------*/
static void HALW5500_Write(uint8_t wb)
{
  HAL_SPIEthWriteRead(wb);
}
/*---------------------------------------------------------------------------*/
bool HALW5500_Init(uint8_t *mac)
{
  DEBUG("HALW5500: MAC ");
  for (uint8_t i = 0;i <= 5;i++) 
    DEBUG("%02X",  mac[i]);
  DEBUG("\n");
  memset(&netInfo, 0, sizeof(netInfo));
  memcpy(netInfo.mac, mac, 6);
  HAL_EthReset();
  reg_wizchip_cs_cbfunc(HAL_EthCS0, HAL_EthCS1);
  reg_wizchip_spi_cbfunc(HALW5500_Read, HALW5500_Write);
  uint8_t memsize[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};
  return ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) >= 0;
}
/*---------------------------------------------------------------------------*/
bool HALW5500_IsLinkUp(void)
{
  uint8_t state;
  ctlwizchip(CW_GET_PHYLINK, (void*) &state);
  //DEBUG("HALW5500_ Link is %u\n", state);
  return state == PHY_LINK_ON;
}
/*---------------------------------------------------------------------------*/
bool HALW5500_WaitForLinkUp(int16_t timeoutS)
{
  DEBUG("HALW5500: Waiting for link...");
  timeoutS *= 10;
  while (timeoutS--) {
    if (HALW5500_IsLinkUp()) {
      DEBUG(" stabel\n");
      return true;
    }
    HAL_DelayMS(100);
  }
  DEBUG(" timeout!\n");
  return false;
}
/*---------------------------------------------------------------------------*/
int8_t HALW5500_Config(const char *config, uint8_t configNumber)
{
  bool Match(const char *text)
  {
    int length = strlen(config);
    return length && (strncmp(config, text, length) == 0);
  }

  netmode_type mode = wizchip_getnetmode();
  if (Match("FORCEARP")) 
    mode |= NM_FORCEARP;
  else if (Match("~FORCEARP")) 
    mode &= ~NM_FORCEARP;
  else if (Match("PINGBLOCK")) 
    mode |= NM_PINGBLOCK;
  else if (Match("~PINGBLOCK")) 
    mode &= ~NM_PINGBLOCK;
  return wizchip_setnetmode(mode);
}
/*---------------------------------------------------------------------------*/
int8_t HALW5500_SetIP(uint32_t ip, uint32_t mask)
{
  char ipString[16];
  IPToString(ipString, sizeof(ipString), ip);
  char maskString[16];
  IPToString(maskString, sizeof(maskString), mask);
  DEBUG("HALW5500: IP %s Mask %s\n",  ipString,  maskString);
  *((uint32_t*) &netInfo.ip) = ip;
  *((uint32_t*) &netInfo.sn) = mask;
  netInfo.dhcp = NETINFO_STATIC;
  return ctlnetwork(CN_SET_NETINFO, (void*) &netInfo);
}
/*---------------------------------------------------------------------------*/
int8_t HALW5500_SetGateway(uint32_t gateway)
{
  char gatewayString[16];
  IPToString(gatewayString, sizeof(gatewayString), gateway);
  DEBUG("HALW5500: Gateway %s\n", gatewayString);
  *((uint32_t*) &netInfo.gw) = gateway;
  return ctlnetwork(CN_SET_NETINFO, (void*) &netInfo);
}
/*---------------------------------------------------------------------------*/
