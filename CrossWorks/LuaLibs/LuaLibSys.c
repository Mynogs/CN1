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
#include "LuaLibSys.h"
#include <float.h>
#include <math.h>
#include "../HAL/HAL.h"
#include "../lua-5.3.0/src/lua.h"
#include "../lua-5.3.0/src/lauxlib.h"
#include "../lua-5.3.0/src/ldebug.h"
#include "../lua-5.3.0/src/lobject.h"
#include "../lua-5.3.0/src/lstate.h"
#include "../lua-5.3.0/src/lopcodes.h"
#include "../SysLog.h"
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
void __attribute__ ((weak)) CNServeTick(lua_State *L) {}
/*---------------------------------------------------------------------------*/
// Lua lib sys
/*---------------------------------------------------------------------------*/
extern uint8_t __stack_start__, __stack_end__;
extern uint8_t __heap_start__, __heap_end__;

LIB_FUNCTION(LuaInfo, "J.")
{
  char *s =
    "["
    "{\"type\":\"CN\"}, "
    "{\"name\":\"Bare-Metal CN " __DATE__ " " __TIME__ "\"},"
    "{\"heapSize\":%d},"
    "{\"heapUseage\":%d},"
    "{\"stackSize\":%d},"
    "{\"stackUsage\":%d}"
    "]";
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_gc(L, LUA_GCCOLLECT, 0);
  lua_pushfstring(
    L,
    s,
    &__heap_end__ - &__heap_start__,
    lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc (L, LUA_GCCOUNTB, 0),
    &__stack_end__ - &__stack_start__,
    HAL_GetStackUsage()
  );
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaDescription, "J.")
{ // TODO NOGS_TYPE
  char *s =
    "["
    "{"
    "\"class\":\"CN\", "
    "\"name\":\"Bare-Metal CN\", "
    "\"version\":\"1.2c\", "
    "\"vendor\":\"Ing. B\\u00FCro Riesberg GmbH\", "  // Note the UTF-8 'ü'
    "\"build\":\"" __DATE__ " " __TIME__ "\""
    "}"
    "]";
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushstring(L, s);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaStop, ".")
{
  LuaUtils_CheckParameter1(L, 0, 0);
  CN_Stop();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaRestart, ".")
{
  LuaUtils_CheckParameter1(L, 0, 0);
  CN_Restart();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaReboot, ".")
{
  LuaUtils_CheckParameter1(L, 0, 0);
  HAL_Reboot();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaNorm, ".V|V,range|V,min,max")
{
  int n =  LuaUtils_CheckParameter2(L, 1, 3, LuaNormUsage);
  double value = luaL_checknumber(L, 1);
  if (n >= 2) {
    double range = n >= 3 ? luaL_checknumber(L, 3) - luaL_checknumber(L, 2) : luaL_checknumber(L, 2);
    value = fabs(range) <= DBL_EPSILON ? 0.0 : value / range;
  }
  if (value < 0.0) 
    value = 0.0; 
  else if (value > 1.0) 
    value = 1.0;
  lua_pushnumber(L, value);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSleep, ".[V]")
{
  LuaUtils_CheckParameter1(L, 1, LuaSleepUsage);
  double time = luaL_checknumber(L, 1);
  if (time > 0.0)
    HAL_DelayMS(time * 1000.0);
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSetStopOnError, ".B")
{
  LuaUtils_CheckParameter1(L, 1, LuaSetStopOnErrorUsage);
  LuaUtils_SetStopOnError(lua_toboolean(L, 1));
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaIsTicked, "B[,ellapsed].")
{
  LuaUtils_CheckParameter1(L, 0, LuaSetStopOnErrorUsage);
  if (HAL_TickIsTicked()) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, HAL_TickGetElapsedTime());
    return 2;
  }
  lua_pushboolean(L, false);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaShow, ".L")
{
  LuaUtils_CheckParameter1(L, 1, LuaShowUsage);
  luaL_checktype(L, 1, LUA_TTABLE);
  LuaUtils_DumpTable(L);
  return 0;
}
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  LIB_FUNCTION(LuaWatchdogInit, "true|E.timeS")
  {
    LuaUtils_CheckParameter1(L, 1, LuaWatchdogInitUsage);
    double timeS = luaL_checknumber(L, 1);
    int16_t result = HAL_WatchdogInit(timeS);  
    if (result < 0) 
      return LuaUtils_Error(L, HAL_ErrorToString(result));
    lua_pushboolean(L, true);
    return 1;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  LIB_FUNCTION(LuaWatchdogServe, ".")
  {
    LuaUtils_CheckParameter1(L, 0, LuaWatchdogServeUsage);
    HAL_WatchdogServe(HAL_WATCHDOG_SERVE_KEY); 
    return 0;
  }
#endif
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "sys")
  LIB_ITEM_FUNCTION("description", LuaDescription)
  LIB_ITEM_FUNCTION("info", LuaInfo)
  LIB_ITEM_FUNCTION("stop", LuaStop)
  LIB_ITEM_FUNCTION("restart", LuaRestart)
  LIB_ITEM_FUNCTION("reboot", LuaReboot)
  LIB_ITEM_FUNCTION("norm", LuaNorm)
  LIB_ITEM_FUNCTION("sleep", LuaSleep)
  LIB_ITEM_FUNCTION("setStopOnError", LuaSetStopOnError)
  LIB_ITEM_FUNCTION("isTicked", LuaIsTicked)
  LIB_ITEM_FUNCTION("show", LuaShow)
  #ifdef HAS_WATCHDOG
    LIB_ITEM_FUNCTION("watchdogInit", LuaWatchdogInit)
    LIB_ITEM_FUNCTION("watchdogServe", LuaWatchdogServe)
  #endif
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibSys_Register(lua_State *L)
{
  LuaUtils_Register(L, 0, "sys", Lib);
}
/*---------------------------------------------------------------------------*/
