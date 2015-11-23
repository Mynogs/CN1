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
#include "LuaLibSoftCounter.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "LuaLibPin.h"
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
/*!
 * \return Open soft Counter
 */
LIB_FUNCTION(LuaOpen, "true.")
{
  LuaUtils_CheckParameter1(L, 0, LuaOpenUsage);
  HAL_SoftCounterInit();
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Close
 */
LIB_FUNCTION(LuaClose, ".")
{
  LuaUtils_CheckParameter1(L, 0, LuaCloseUsage);
  HAL_SoftCounterDeinit();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaConfig, "true|E.P{,!}")
{
  int n = LuaUtils_CheckParameter2(L, 2, 254, LuaConfigUsage);
  int16_t pinId = LuaLibPin_ToId(L, 1);
  if (pinId < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(pinId));
  for (uint8_t i = 2; i <= n; i++) {
    const char *config = luaL_checkstring(L, i);
    int16_t result = HAL_SoftCounterConfig(pinId, config, i - 2);
    if (result < 0)
      return LuaUtils_Error(L, HAL_ErrorToString(result));
  }
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGet, "V|E.P,{V}}")
{
  int n = LuaUtils_CheckParameter2(L, 1, 2, LuaGetUsage);
  int16_t pinId = LuaLibPin_ToId(L, 1);
  int64_t value;
  int16_t result;
  if (n >= 2) {
    int64_t set = luaL_checkinteger(L, 2);
    result = HAL_SoftCounterGet(pinId, &value, &set);
  } else 
    result = HAL_SoftCounterGet(pinId, &value, 0);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushinteger(L, value);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "softCounter")
  LIB_ITEM_FUNCTION("open",LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("config", LuaConfig)
  LIB_ITEM_FUNCTION("get", LuaGet)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibSoftCounter_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "softCounter", Lib);
}
/*---------------------------------------------------------------------------*/

