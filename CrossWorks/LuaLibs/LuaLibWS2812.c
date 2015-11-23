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
#include "LuaLibWS2812.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "../WS2812/WS2812.h"
#include "../Memory.h"
#include "../lua-5.3.0/src/lua.h"
#include "../lua-5.3.0/src/lualib.h"
#include "../lua-5.3.0/src/lauxlib.h"
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
static uint8_t Number01ToByte(double x)
{
  if (x <= 0.0) 
    return 0;
  if (x >= 1.0) 
    return 255;
  return x * 255.0;
}
/*---------------------------------------------------------------------------*/
// Display three LEDs in R, G and B
// ws2812.send(name | pinHandle, '\xFF\x00\x00\x00\xFF\x00\x00\x00\xFF')
// ws2812.send(name | pinHandle, 1, 0, 0, 0, 1, 0, 0, 0, 1)
// ws2812.send(name | pinHandle, {1, 0, 0, 0, 1, 0, 0, 0, 1})
// ws2812.send(name | pinHandle, {1, 0, 0, {0, 1, 0}, 0, 0, 1})
// ws2812.send(name | pinHandle, {{1, 0, 0}, {0, 1, 0}, {0, 0, 1})
LIB_FUNCTION(LuaSend, ".P|H,S|[{red,green,blue}|array3]")
{
  int n = LuaUtils_CheckParameter2(L, 2, 769, LuaSendUsage);
  int16_t pinId = LuaLibPin_ToId(L, 1);
  if (pinId < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(pinId));
  uint8_t *rgb;
  bool_t rgbFree = false;
  if (n == 2) {
    if (lua_type(L, 2) == LUA_TSTRING) {
      rgb = (uint8_t*) lua_tolstring(L, 2, &n);
      if (n % 3) 
        return 
      LuaUtils_Error(L, "String must contains triplets");
    } else {
      luaL_checktype(L, 2, LUA_TTABLE);
      n = 0;
      for (uint16_t i = 0; i < 768; i++) {
        int type = lua_rawgeti(L, 2, i + 1);
        if (type == LUA_TNIL) 
          break;
        if ((type == LUA_TTABLE) && (n % 3))
          return LuaUtils_Error(L, "missalign RGB sub array at %u. Must be triplets", i);
        n += type == LUA_TTABLE ? 3 : 1;
        lua_pop(L, 1);
      }
      rgb = (uint8_t*) Memory_Allocate(n, 0);
      rgbFree = true;
      if (!rgb) 
        return LuaUtils_Error(L, "out of memory");
      uint8_t *rgb2 = rgb;
      for (uint16_t i = 0; true; i++) {
        int type = lua_rawgeti(L, 2, i + 1);
        if (type == LUA_TNIL) 
          break;
        if (type == LUA_TTABLE) {
          for (uint8_t i = 0; i <= 2; i++) {
            lua_rawgeti(L, -1, i + 1);
            *rgb2++ = Number01ToByte(luaL_checknumber(L, -1));
            lua_pop(L, 1);
          }
        } else
          *rgb2++ = Number01ToByte(luaL_checknumber(L, -1));
        lua_pop(L, 1);
      }
    }
  } else {
    n = (n - 1) / 3 * 3;
    rgb = Memory_Allocate(n, 0);
    rgbFree = true;
    if (!rgb)
      return LuaUtils_Error(L, "out of memory");
    uint8_t *rgb2 = rgb;
    for (uint16_t i = 0; i < n; i++)
      *rgb2++ = Number01ToByte(luaL_checknumber(L, i + 2));
  }
  WS2812_Send(rgb, n, false, pinId);
  if (rgbFree) 
    Memory_Free(rgb, 0);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "ws2812")
  LIB_ITEM_FUNCTION("send", LuaSend)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibWS2812_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "ws2812", Lib);
}
/*---------------------------------------------------------------------------*/










