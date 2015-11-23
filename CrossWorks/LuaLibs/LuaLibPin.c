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
// Frontier Version
/*---------------------------------------------------------------------------*/
#include "LuaLibPin.h"
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
static void* IdToHandle(int16_t id)
{
  return (void*) (id | (0xFEAD << 16));
}
/*---------------------------------------------------------------------------*/
int16_t LuaLibPin_ToId(lua_State *L, int i)
{
  if (lua_isstring(L, i))
    return HAL_PinNameToId(luaL_checkstring(L, i));
  uint32_t handle = (uint32_t) lua_touserdata(L, i);
  if (handle >> 16 != 0xFEAD)
    return RESULT_ERROR_BAD_HANDEL;
  return handle & 0xFFFF;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaConfig, "H|E.N{,!}")
{
  int n = LuaUtils_CheckParameter2(L, 1, 254, LuaConfigUsage);
  const char *name = luaL_checkstring(L, 1);
  int16_t pinId = HAL_PinNameToId(name);
  if (pinId < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(pinId));
  for (uint8_t i = 2;i <= n;i++) {
    const char *config = luaL_checkstring(L, i);
    int16_t result = HAL_PinConfig(pinId, config, i - 2);
    if (result < 0)
      return LuaUtils_Error(L, HAL_ErrorToString(result));
  }
  lua_pushlightuserdata(L, IdToHandle(pinId));
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSet,".N|H,high")
{
  LuaUtils_CheckParameter1(L, 2, LuaSetUsage);
  int16_t pinId = LuaLibPin_ToId(L, 1);
  if (pinId < 0)
    return LuaUtils_Error(L, "unknown pin name");
  bool high = lua_toboolean(L, 2);
  HAL_PinSet(pinId, high);
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGet, "high|E.N")
{
  LuaUtils_CheckParameter1(L, 1, LuaGetUsage);
  int16_t pinId = LuaLibPin_ToId(L, 1);
  if (pinId < 0)
    return LuaUtils_Error(L, "unknown pin name");
  lua_pushboolean(L, HAL_PinGet(pinId));
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGetNames, "S.")
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushstring(L, HAL_PinGetNames());
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "pin")
  LIB_ITEM_FUNCTION("config", LuaConfig)
  LIB_ITEM_FUNCTION("set", LuaSet)
  LIB_ITEM_FUNCTION("get", LuaGet)
  LIB_ITEM_FUNCTION("getNames", LuaGetNames)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibPin_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "pin", Lib);
}
/*---------------------------------------------------------------------------*/

