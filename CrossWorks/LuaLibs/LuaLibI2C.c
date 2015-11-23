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
/* I2C special addresses:
  Addr        RW Bit    Description
  000 0000      0      General call address
  000 0000      1      START byte(1)
  000 0001      X      CBUS address(2)
  000 0010      X      Reserved for different bus format (3)
  000 0011      X      Reserved for future purposes
  000 01XX      X      Hs-mode master code
                        Normal addresses
  111 10XX      X      10-bit slave addressing
  111 11XX      X      Reserved for future purposes
*/
/*---------------------------------------------------------------------------*/
#include "LuaLibI2C.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "LuaLibUtils.h"
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
LIB_FUNCTION(LuaOpen, "true|E.C{,!}")
{
  int n = LuaUtils_CheckParameter2(L, 1, 254, LuaOpenUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  int8_t result = HAL_I2COpen(channel);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  for (uint8_t i = 2;i <= n;i++) {
    const char *config = luaL_checkstring(L, i);
    int8_t result = HAL_I2CConfig(channel, config);
    if (result < 0) 
      return LuaUtils_Error(L, HAL_ErrorToString(result));
  }
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaClose, ".C")
{
  LuaUtils_CheckParameter1(L, 1, LuaCloseUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  HAL_I2CClose(channel);
  return 0;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Scan the bus for existing devices.
 * Is no address specified the bus is scannend from 8 to 123.
 * If one address, this address is checked
 * With two addresses the range is scanned. Then it is possible to scan 
 * from 1 to 127
 * Result is a bool value, true if min one device found, false is there
 * are no devices. Followed by a list of existing devices. 
 * Or, a standart error result.
 */
LIB_FUNCTION(LuaExists, "B{,addr}|E.C[,addr1[,addr2]]")
{
  int n = LuaUtils_CheckParameter2(L, 1, 3, LuaExistsUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  uint8_t addr1, addr2;
  if (n > 1) {
    addr1 = luaL_checkinteger(L, 2);
    addr2 = n >= 3 ? luaL_checkinteger(L, 3) : addr1;
  } else 
    addr1 = 8, addr2 = 122;
  uint64_t present[2];
  memset(present, 0, sizeof(present));
  for (uint8_t addr = addr1; addr <= addr2; addr++) {
    int16_t result = HAL_I2CExist(channel, addr);
    if (result < 0) {
      if (result != RESULT_ERROR_TIMEOUT)
        return LuaUtils_Error(L, HAL_ErrorToString(result));
    } else 
      present[addr >> 6] |= 1LL << (addr & 0x3F);
  }
  lua_pushboolean(L, present[0] || present[1]);
  n = 1;
  for (uint8_t addr = 0; addr <= 127; addr++) 
    if (present[addr >> 6] & (1LL << (addr & 0x3F)))
      lua_pushinteger(L, addr), n++;
  return n;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSend, "count|E.C,addr[,iaddre,iaddrsize=1],send)")
{
  int n = LuaUtils_CheckParameter2(L, 3, 5, LuaSendUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  uint8_t addr = luaL_checkinteger(L, 2);
  int32_t internalAddr = n >= 4 ? luaL_checkinteger(L, 3) : 0;
  uint8_t internalAddrSize = n >= 5 ? luaL_checkinteger(L, 4) : 0;
  const char *send = luaL_checkstring(L, n);
  uint32_t size = lua_rawlen(L, n);
  int result = HAL_I2CSend(channel, addr, internalAddr, internalAddrSize, send, size);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  return result;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaRecv, "recv|E.C,addr[,iaddre,iaddrsize=1],count")
{
  int n = LuaUtils_CheckParameter2(L, 3, 5, LuaRecvUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  uint8_t addr = luaL_checkinteger(L, 2);                       //SW Before: uint8_t addr = luaL_checkinteger(L, n); --> n is not the place of the address
  int32_t internalAddr = n >= 4 ? luaL_checkinteger(L, 3) : 0;
  uint8_t internalAddrSize = n >= 5 ? luaL_checkinteger(L, 4) : 0;
  uint32_t count = luaL_checkinteger(L, n);
  luaL_Buffer buffer;
  char *recv = luaL_buffinitsize(L, &buffer, count);
  int16_t result = HAL_I2CRecv(channel, addr, internalAddr, internalAddrSize, recv, count);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  luaL_pushresultsize(&buffer, count);                          //SW Before: luaL_pushresultsize(&buffer, result); --> result is not the size of the buffer
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "i2c")
  LIB_ITEM_FUNCTION("open", LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("exists", LuaExists)
  LIB_ITEM_FUNCTION("send", LuaSend)
  LIB_ITEM_FUNCTION("recv", LuaRecv)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibI2C_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "i2c", Lib);
}
/*---------------------------------------------------------------------------*/
