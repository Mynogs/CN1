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
#include "LuaLibBASE64.h"
#include "../Common.h"
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
/*---------------------------------------------------------------------------

        |     Byte0     |     Byte1     |     Byte2     |
        |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
        |           |           |           |           |
        |5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|
        |   Char0   |   Char1   |   Char2   |   Char3   |

  Samples:

  A       QQ==
  AA      QUE=
  AAA     QUFB
  AAAA    QUFBQQ==

  Base64 beschreibt ein Verfahren zur Kodierung
    QmFzZTY0IGJlc2NocmVpYnQgZWluIFZlcmZhaHJlbiB6dXIgS29kaWVydW5n

  Base64 beschreibt ein Verfahren zur Kodierung!
    QmFzZTY0IGJlc2NocmVpYnQgZWluIFZlcmZhaHJlbiB6dXIgS29kaWVydW5nIQ==

  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define NIL 0x80
/*---------------------------------------------------------------------------*/
uint8_t Encode(uint8_t i6)
{
  static const uint8_t t[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" // 0..25
    "abcdefghijklmnopqrstuvwxyz" // 26..51
    "0123456789"                 // 52..61
    "+/";                        // 62, 63
  return t[i6];
}
/*---------------------------------------------------------------------------*/
uint8_t Decode(uint8_t i8)
{
  static const uint8_t t[256 - 2 * 16] = {
    NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, 62 , NIL, 62 , NIL, 63,  // sp ! " # $ % & ' ( ) * + , - . /
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  NIL, NIL, NIL, 0,   NIL, NIL, // 0..9 : ; < = > ?
    NIL, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  // @..O
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  NIL, NIL, NIL, NIL, 63,  // P..Z [ \ ] ^ _
    NIL, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  // `..o
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51 , NIL, NIL, NIL, NIL, NIL  // p..z { | } ~ del
  };
  return i8 >= 32 ? t[i8 - 32] : NIL;
}
/*---------------------------------------------------------------------------*/
static uint32_t temp;


// Binär -> BASE64
LIB_FUNCTION(LuaEncode, "S.(S|I)")
{
  LuaUtils_CheckParameter1(L, 1, LuaEncodeUsage);
  /*const*/ uint8_t *s;
  int sLength;
  if (lua_type(L, 1) == LUA_TSTRING) {
    s = luaL_checkstring(L, 1);
    sLength = strlen(s);
  } else {
    uint32_t v  = luaL_checkinteger(L, 1);
    luaL_Buffer tBuffer;
    char *t = luaL_buffinitsize(L, &tBuffer, 4);
    *t++ = Encode(v >> 18);
    *t++ = Encode(v >> 12);
    *t++ = Encode(v >> 6);
    *t++ = Encode(v);
    luaL_pushresultsize(&tBuffer, 4);
    return 1;
  }
  int tLength = (sLength + 2) / 3 * 4;
  luaL_Buffer tBuffer;
  char *t = luaL_buffinitsize(L, &tBuffer, tLength);
  while (sLength >= 3) {
    *t++ = Encode(s[0] >> 2);
    *t++ = Encode(((s[0] & 0x03) << 4) | (s[1] >> 4));
    *t++ = Encode(((s[1] & 0x0f) << 2) | (s[2] >> 6));
    *t++ = Encode(s[2] & 0x3f);
    s += 3;
    sLength -= 3;
  }
  if (sLength) {
    *t++ = Encode(s[0] >> 2);
    if (sLength == 1) {
      *t++ = Encode((s[0] & 0x03) << 4);
      *t++ = '=';
    } else if (sLength == 2) {
      *t++ = Encode(((s[0] & 0x03) << 4) | (s[1] >> 4));
      *t++ = Encode((s[1] & 0x0f) << 2);
    }
    *t++ = '=';
  }
  luaL_pushresultsize(&tBuffer, tLength);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaDecode, "S|E.S")
{
  char errorChar = 0;

  uint8_t DecodeAndCheck(uint8_t c)
  {
    char c2 = Decode(c);
    if (c2 == NIL)
      errorChar = c;
    return c2;
  }

  LuaUtils_CheckParameter1(L, 1, LuaDecodeUsage);
  const char *s = luaL_checkstring(L, 1);
  int sLength = strlen(s);
  if (!sLength) {
    lua_pushstring(L, "");
    return 1;
  }
  if (sLength & 3) return LuaUtils_Error(L, "Wrong length");
  luaL_Buffer tBuffer;
  int tLength = sLength / 4 * 3;
  if (s[sLength - 1] == '=') tLength--;
  if (s[sLength - 2] == '=') tLength--;
  uint8_t *t = luaL_buffinitsize(L, &tBuffer, (sLength >> 2) + (sLength >> 1));
  while (sLength >= 4) {
    *t++ = (DecodeAndCheck(s[0]) << 2) | (DecodeAndCheck(s[1]) >> 4);
    *t++ = (DecodeAndCheck(s[1]) << 4) | (DecodeAndCheck(s[2]) >> 2);
    *t++ = (DecodeAndCheck(s[2]) << 6) | DecodeAndCheck(s[3]);
    if (errorChar) return LuaUtils_Error(L, "Illegal char '%c' ASCII(%u)", errorChar, errorChar);
    s += 4;
    sLength -= 4;
  }
  luaL_pushresultsize(&tBuffer, tLength);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "base64")
  LIB_ITEM_FUNCTION("encode", LuaEncode)
  LIB_ITEM_FUNCTION("decode", LuaDecode)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibBASE64_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "base64", Lib);
}
/*---------------------------------------------------------------------------*/
