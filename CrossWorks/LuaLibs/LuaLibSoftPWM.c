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
#include "LuaLibSoftPWM.h"
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
 * \return Open soft PWM
 */
LIB_FUNCTION(LuaOpen, "true|E.baseclock,Cs")
{
  LuaUtils_CheckParameter1(L, 2, LuaOpenUsage);
  int baseClockHz = luaL_checkinteger(L, 1);
  int channelCount = luaL_checkinteger(L, 2);
  int16_t result = HAL_SoftPWMInit(baseClockHz, channelCount);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
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
  HAL_SoftPWMDeinit();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaConfig, "true|E.C,P")
{
  LuaUtils_CheckParameter1(L, 2, LuaConfigUsage);
  int channelNumber = luaL_checkinteger(L, 1);
  int16_t pinId = LuaLibPin_ToId(L, 2);
  if (pinId < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(pinId));
  int16_t result = HAL_SoftPWMConfig(channelNumber, pinId);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSet, "true|E.C,[intervall|intervall0,intervall1]}")
{
  int n = LuaUtils_CheckParameter2(L, 2, 3, LuaSetUsage);
  int channelNumber = luaL_checkinteger(L, 1);
  double intervalLowNS = luaL_checknumber(L, 2);
  double intervalHighNS;
  if (n >= 3)
    intervalHighNS = luaL_checknumber(L, 3);
  else {
    intervalLowNS /= 2.0;
    intervalHighNS = intervalLowNS;
  }
  if ((intervalLowNS <= 0.0) || (intervalLowNS > 4.0) || (intervalHighNS <= 0.0) || (intervalHighNS > 4.0))
    return LuaUtils_Error(L, HAL_ErrorToString(RESULT_HAL_ERROR_WRONG_PARAMETER));
  int16_t result = HAL_SoftPWMSet(channelNumber, intervalLowNS * 1.0E9, intervalHighNS * 1.0E9);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "softPWM")
  LIB_ITEM_FUNCTION("open",LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("config", LuaConfig)
  LIB_ITEM_FUNCTION("set", LuaSet)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibSoftPWM_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "softPWM", Lib);
}
/*---------------------------------------------------------------------------*/

