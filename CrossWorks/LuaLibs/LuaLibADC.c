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
#include "LuaLibADC.h"
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
/*!
 * \return Open ADC
 */
LIB_FUNCTION(LuaOpen, "true|E.[calibrate]")
{
  int n = LuaUtils_CheckParameter2(L, 0, 1, LuaOpenUsage);
  int16_t result = HAL_ADCInit(false);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Close ADC
 */
LIB_FUNCTION(LuaClose, ".")
{
  LuaUtils_CheckParameter1(L, 0, LuaCloseUsage);
  HAL_ADCDeinit();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaConfig, "true|E.C{,!}")
{
  int n = LuaUtils_CheckParameter2(L, 2, 254, LuaConfigUsage);
  int channelNumber = luaL_checkinteger(L, 1);
  for (uint8_t i = 2;i <= n;i++) {
    const char *config = luaL_checkstring(L, i);
    int16_t result = HAL_ADCConfig(channelNumber, config, i - 2);
    if (result < 0) 
      return LuaUtils_Error(L, HAL_ErrorToString(result));
  }
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGet, "{V}|E.{C}")
{
  int n = LuaUtils_CheckParameter2(L, 0, 254, LuaGetUsage);
  if (n) {
    for (uint8_t i = 0; i < n; i++) {
      uint32_t channelNumber = luaL_checkinteger(L, i + 1);
      float value;
      int16_t result = HAL_ADCGet(channelNumber, &value);
      if (result < 0) 
        return LuaUtils_Error(L, HAL_ErrorToString(result));
      lua_pushnumber(L, value);
    }
    return n;
  }
  int16_t result = HAL_ADCGetAll(0);
  if (result > 0) {
    float values[result];
    result = HAL_ADCGetAll(values);
    for (int8_t i = 0; i < result; i++)
      lua_pushnumber(L, values[i]);
  }
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  return result;  
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "adc")
  LIB_ITEM_FUNCTION("open",LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("config", LuaConfig)
  LIB_ITEM_FUNCTION("get", LuaGet)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibADC_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "adc", Lib);
}
/*---------------------------------------------------------------------------*/

