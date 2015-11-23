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
#include "LuaLibSPI.h"
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
LIB_FUNCTION(LuaOpen, "treu|E.C{,!}")
{
  int n = LuaUtils_CheckParameter2(L, 1, 254, LuaOpenUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  int16_t result = HAL_SPIOpen(channel);
  if (result < 0) 
    return LuaUtils_Error(L, "Channel not avail");
  for (uint8_t i = 2; i <= n; i++) {
    const char *config = luaL_checkstring(L, i);
    int16_t result = HAL_SPIConfig(channel, config, i - 2);
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
  HAL_SPIClose(channel);
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSend, "count|E.C,send")
{
  LuaUtils_CheckParameter1(L, 2, LuaSendUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  const char *send = luaL_checkstring(L, 2);
  uint32_t size = lua_rawlen(L, 2);
  int32_t result = HAL_SPISend(channel, send, size);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  return result;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaRecv, "recv|E.C")
{
  LuaUtils_CheckParameter1(L, 1, LuaRecvUsage);
  uint32_t channel = luaL_checkinteger(L, 1);
  int32_t result = HAL_SPIRecv(channel, 0, 0);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  luaL_Buffer buffer;
  char *recv = luaL_buffinitsize(L, &buffer, result);
  result = HAL_SPIRecv(channel, recv, result);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  luaL_pushresultsize(&buffer, result);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "spi")
  LIB_ITEM_FUNCTION("open", LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("send", LuaSend)
  LIB_ITEM_FUNCTION("recv", LuaRecv)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibSPI_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "spi", Lib);
}
/*---------------------------------------------------------------------------*/
