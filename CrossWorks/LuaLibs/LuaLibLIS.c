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
#include "LuaLibLIS.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "../HAL/HALLIS.h"
#include "LuaLibUtils.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
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
LIB_FUNCTION(LuaOpen, "true|E.")
{
  LuaUtils_CheckParameter1(L, 0, LuaOpenUsage);
  int16_t result = HALLIS_Open();
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaClose, ".")
{
  LuaUtils_CheckParameter1(L, 0, LuaCloseUsage);
  HALLIS_Close();
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGetXYZ, "x,y,z.")
{
  LuaUtils_CheckParameter1(L, 0, LuaGetXYZUsage);
  XYZ16 xyz;
  int16_t result = HALLIS_GetXYZ(&xyz);
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  lua_pushinteger(L, xyz.x);
  lua_pushinteger(L, xyz.y);
  lua_pushinteger(L, xyz.z);
  return 3;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaGetNoise, "noise|E.")
{
  LuaUtils_CheckParameter1(L, 0, LuaGetNoise);
  int32_t result = HALLIS_GetNoise();
  if (result < 0) 
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "lis")
  LIB_ITEM_FUNCTION("open", LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("getXYZ", LuaGetXYZ)
  LIB_ITEM_FUNCTION("getNoise", LuaGetNoise)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibLIS_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "lis", Lib);
}
/*---------------------------------------------------------------------------*/
