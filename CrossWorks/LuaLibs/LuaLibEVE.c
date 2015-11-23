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
// EVE
/*---------------------------------------------------------------------------*/
// Config
#define HAS_RAW
/*---------------------------------------------------------------------------*/
#include "LuaLibEVE.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "LuaLibUtils.h"
#include "FatFs-0-10-c/src/ff.h"
#include "Memory.h"
#include "EVE/EVE.h"
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
#define RESULT_LIBEVE_ERROR_WRONG_DATATYPE -100
/*---------------------------------------------------------------------------*/
#define FT_CMD_SIZE 4
/*---------------------------------------------------------------------------*/
/*!
 * \brief Coprocessor engine - Command Encoding
 * The list in FT800.h is incomplete, so use my own list!
 */
/*---------------------------------------------------------------------------*/
// These commands begin and finish the display list:
#define CMD_DLSTART          0xFFFFFF00 // start a new display list
#define CMD_SWAP             0xFFFFFF01 // swap the current display list
/*---------------------------------------------------------------------------*/
// Commands to draw graphics objects:
#define CMD_BGCOLOR          0xFFFFFF09 // set the background colour
#define CMD_FGCOLOR          0xFFFFFF0A // set the foreground colour
#define CMD_GRADIENT         0xFFFFFF0B // draw a smooth colour gradient
#define CMD_TEXT             0xFFFFFF0C // draw text
#define CMD_BUTTON           0xFFFFFF0D // draw a button
#define CMD_KEYS             0xFFFFFF0E // draw a row of keys
#define CMD_GRADCOLOR        0xFFFFFF34 // set the 3D effects for CMD_BUTTON and CMD_KEYS highlight colour
#define CMD_PROGRESS         0xFFFFFF0F // draw a progress bar
#define CMD_SLIDER           0xFFFFFF10 // draw a slider
#define CMD_SCROLLBAR        0xFFFFFF11 // draw a scroll bar
#define CMD_TOGGLE           0xFFFFFF12 // draw a toggle switch
#define CMD_GAUGE            0xFFFFFF13 // draw a gauge
#define CMD_CLOCK            0xFFFFFF14 // draw an analogue clock
#define CMD_DIAL             0xFFFFFF2D // draw a rotary dial control
#define CMD_NUMBER           0xFFFFFF2E // draw a decimal number
/*---------------------------------------------------------------------------*/
// Commands to operate on memory:
#define CMD_MEMCRC           0xFFFFFF18 // compute a CRC-32 for memory
#define CMD_MEMWRITE         0xFFFFFF1A // write bytes into memory
#define CMD_MEMSET           0xFFFFFF1B // fill memory with a byte value
#define CMD_MEMZERO          0xFFFFFF1C // write zero to a block of memory
#define CMD_MEMCPY           0xFFFFFF1D // copy a block of memory
#define CMD_APPEND           0xFFFFFF1E // append memory to display list
/*---------------------------------------------------------------------------*/
// Commands for loading image data into FT800 memory:
#define CMD_INFLATE          0xFFFFFF22 // inflate data into memory
#define CMD_GETPTR           0xFFFFFF23 // Get the end memory address of inflated data
#define CMD_LOADIMAGE        0xFFFFFF24 // load a JPEG image
/*---------------------------------------------------------------------------*/
// Commands for setting the bitmap transform matrix:
#define CMD_BITMAP_TRANSFORM 0xFFFFFF21 // bitmap transform A-F
#define CMD_LOADIDENTITY     0xFFFFFF26 // set the current matrix to identity
#define CMD_TRANSLATE        0xFFFFFF27 // apply a translation to the current matrix
#define CMD_SCALE            0xFFFFFF28 // apply a scale to the current matrix
#define CMD_ROTATE           0xFFFFFF29 // apply a rotation to the current matrix
#define CMD_SETMATRIX        0xFFFFFF2A // write the current matrix as a bitmap transform
#define CMD_GETMATRIX        0xFFFFFF33 // retrieves the current matrix coefficients
/*---------------------------------------------------------------------------*/
// Touch Commands commands:
#define CMD_CALIBRATE        0xFFFFFF15 // execute the touch screen calibration routine
#define CMD_TOUCH_TRANSFORM  0xFFFFFF20 // touch Transform A-F
#define CMD_TRACK            0xFFFFFF2C // track touches for a graphics object
/*---------------------------------------------------------------------------*/
// Other commands:
#define CMD_LOGO             0xFFFFFF31 // play device logo animation
#define CMD_COLDSTART        0xFFFFFF32 // set co-processor engine state to default values
#define CMD_INTERRUPT        0xFFFFFF02 // trigger interrupt INT_CMDFLAG
#define CMD_SPINNER          0xFFFFFF16 // start an animated spinner
#define CMD_STOP             0xFFFFFF17 // stop any spinner, screensaver or sketch
#define CMD_SCREENSAVER      0xFFFFFF2F // start an animated screensaver
#define CMD_SKETCH           0xFFFFFF30 // start a continuous sketch update
#define CMD_SNAPSHOT         0xFFFFFF1F // take a snapshot of the current screen
#define CMD_SETFONT          0xFFFFFF2B // set up a custom font
#define CMD_REGREAD          0xFFFFFF19 // read a register value
#define CMD_GETPROPS         0xFFFFFF25 // read a property value
#define CMD_CRC              0xFFFFFF03 // seems undocumented
#define CMD_HAMMERAUX        0xFFFFFF04 // seems undocumented
#define CMD_MARCH            0xFFFFFF05 // seems undocumented
#define CMD_IDCT             0xFFFFFF06 // seems undocumented
#define CMD_EXECUTE          0xFFFFFF07 // seems undocumented
#define CMD_GETPOINT         0xFFFFFF08 // seems undocumented
/*---------------------------------------------------------------------------*/
int StringToNumber(const char *string, const char *strings[], int defaultValue)
{
  int index = -1;
  for (;;) {
    const char *s = strings[++index];
    if (!s) 
      break;
    if (strcmp(string, s) == 0)
      return index;
  }
  return defaultValue;
}
/*---------------------------------------------------------------------------*/
int StringToNumber2(const char *string, const char *string0, const char *string1, int defaultValue)
{
  if (strcmp(string, string0) == 0)
    return 0;
  if (strcmp(string, string1) == 1)
    return 1;
  return defaultValue;
}
/*---------------------------------------------------------------------------*/
// Lua display list
/*---------------------------------------------------------------------------*/
// DL_ALPHA_FUNC        0x09000000UL // requires OR'd arguments
#define ALPHA_FUNC(func, ref) ((9UL<<24)|(((func)&7UL)<<8)|(((ref)&255UL)<<0))
static int LuaALPHA_FUNC(lua_State *L)
{
  const static char* strings[] = {"NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS", 0};
  int func = StringToNumber(luaL_checkstring(L, 1), strings, 7);
  int ref = luaL_checkinteger(L, 2);
  lua_pushlightuserdata(L, (void*) ALPHA_FUNC(func, ref));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_HANDLE      0x05000000UL // requires OR'd arguments
#define BITMAP_HANDLE(handle) ((5UL<<24)|(((handle)&31UL)<<0))
static int LuaBITMAP_HANDLE(lua_State *L)
{
  int handle = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_HANDLE(handle));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_LAYOUT      0x07000000UL // requires OR'd arguments
#define BITMAP_LAYOUT(format, linestride, height) ((7UL<<24)|(((format)&31UL)<<19)|(((linestride)&1023UL)<<9)|(((height)&511UL)<<0))
static int LuaBITMAP_LAYOUT(lua_State *L)
{
  const static char* strings[] = {"ARGB1555", "L1", "L4", "L8", "RGB332", "ARGB2", "ARGB4", "RGB565", "PALETTED", "TEXT8X8", "TEXTVGA", "BARGRAPH", 0};
  int format = StringToNumber(luaL_checkstring(L, 1), strings, 0);
  int linestride = luaL_checkinteger(L, 2);
  int height = luaL_checkinteger(L, 3);
  lua_pushlightuserdata(L, (void*) BITMAP_LAYOUT(format, linestride, height));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_SIZE        0x08000000UL // requires OR'd arguments
#define BITMAP_SIZE(filter, wrapx, wrapy, width, height) ((8UL<<24)|(((filter)&1UL)<<20)|(((wrapx)&1UL)<<19)|(((wrapy)&1UL)<<18)|(((width)&511UL)<<9)|(((height)&511UL)<<0))
static int LuaBITMAP_SIZE(lua_State *L)
{
  int filter = StringToNumber2(luaL_checkstring(L, 1), "NEAREST", "BILINEAR" , 0);
  int wrapx = StringToNumber2(luaL_checkstring(L, 2), "REPEAT", "BORDER" , 0);
  int wrapy = StringToNumber2(luaL_checkstring(L, 3), "REPEAT", "BORDER" , 0);
  int width = luaL_checkinteger(L, 4);
  int height = luaL_checkinteger(L, 5);
  lua_pushlightuserdata(L, (void*) BITMAP_SIZE(filter, wrapx, wrapy, width, height));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_SOURCE      0x01000000UL // requires OR'd arguments
#define BITMAP_SOURCE(addr) ((1UL<<24)|(((addr)&1048575UL)<<0))
static int LuaBITMAP_SOURCE(lua_State *L)
{
  int addr = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_SOURCE(addr));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_A    0x15000000UL // requires OR'd arguments
#define BITMAP_TFORM_A(a) ((21UL<<24)|(((a)&131071UL)<<0))
static int LuaBITMAP_TFORM_A(lua_State *L)
{
  int a = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_A(a));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_B    0x16000000UL // requires OR'd arguments
#define BITMAP_TFORM_B(b) ((22UL<<24)|(((b)&131071UL)<<0))
static int LuaBITMAP_TFORM_B(lua_State *L)
{
  int b = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_B(b));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_C    0x17000000UL // requires OR'd arguments
#define BITMAP_TFORM_C(c) ((23UL<<24)|(((c)&16777215UL)<<0))
static int LuaBITMAP_TFORM_C(lua_State *L)
{
  int c = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_C(c));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_D    0x18000000UL // requires OR'd arguments
#define BITMAP_TFORM_D(d) ((24UL<<24)|(((d)&131071UL)<<0))
static int LuaBITMAP_TFORM_D(lua_State *L)
{
  int d = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_D(d));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_E    0x19000000UL // requires OR'd arguments
#define BITMAP_TFORM_E(e) ((25UL<<24)|(((e)&131071UL)<<0))
static int LuaBITMAP_TFORM_E(lua_State *L)
{
  int e = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_D(e));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BITMAP_TFORM_F    0x1A000000UL // requires OR'd arguments
#define BITMAP_TFORM_F(f) ((26UL<<24)|(((f)&16777215UL)<<0))
static int LuaBITMAP_TFORM_F(lua_State *L)
{
  int f = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) BITMAP_TFORM_D(f));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BLEND_FUNC        0x0B000000UL // requires OR'd arguments
#define BLEND_FUNC(src, dst) ((11UL<<24)|(((src)&7UL)<<3)|(((dst)&7UL)<<0))
static int LuaBLEND_FUNC(lua_State *L)
{
  const static char* strings[] = {"ZERO", "ONE", "SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_SRC_ALPHA", "ONE_MINUS_DST_ALPHA", 0};
  int src = StringToNumber(luaL_checkstring(L, 1), strings, 2);
  int dst = StringToNumber(luaL_checkstring(L, 2), strings, 4);
  lua_pushlightuserdata(L, (void*) BLEND_FUNC(src, dst));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_BEGIN              0x1F000000UL // requires OR'd arguments
#define BEGIN(prim) ((31UL<<24)|(((prim)&15UL)<<0))
static int LuaBEGIN(lua_State *L)
{
  const static char* strings[] = {"BITMAPS", "POINTS", "LINES", "LINE_STRIP", "EDGE_STRIP_R", "EDGE_STRIP_L", "EDGE_STRIP_A", "EDGE_STRIP_B", "RECTS", 0};
  int prim = 1 + StringToNumber(luaL_checkstring(L, 1), strings, 0);
  lua_pushlightuserdata(L, (void*) BEGIN(prim));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CALL              0x1D000000UL // requires OR'd arguments
#define CALL(dest) ((29UL<<24)|(((dest)&65535UL)<<0))
static int LuaCALL(lua_State *L)
{
  int dest = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) CALL(dest));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CLEAR              0x26000000UL // requires OR'd arguments
#define CLEAR(c, s, t) ((38UL<<24)|(((c)&1UL)<<2)|(((s)&1UL)<<1)|(((t)&1UL)<<0))
static int LuaCLEAR(lua_State *L)
{
  bool c = !!luaL_checkinteger(L, 1);
  bool s = !!luaL_checkinteger(L, 2);
  bool t = !!luaL_checkinteger(L, 3);
  lua_pushlightuserdata(L, (void*) CLEAR(c, s, t));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CELL              0x06000000UL // requires OR'd arguments
#define CELL(cell) ((6UL<<24)|(((cell)&127UL)<<0))
static int LuaCELL(lua_State *L)
{
  int cell = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) CELL(cell));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CLEAR_STENCIL      0x11000000UL // requires OR'd arguments
#define CLEAR_STENCIL(s) ((17UL<<24)|(((s)&255UL)<<0))
static int LuaCLEAR_STENCIL(lua_State *L)
{
  int s = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) CLEAR_STENCIL(s));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CLEAR_TAG          0x12000000UL // requires OR'd arguments
#define CLEAR_TAG(t) ((18UL<<24)|(((t)&255UL)<<0))
static int LuaCLEAR_TAG(lua_State *L)
{
  int t = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) CLEAR_TAG(t));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_COLOR_A            0x0F000000UL // requires OR'd arguments
#define COLOR_A(alpha) ((16UL<<24)|(((alpha)&255UL)<<0))
static int LuaCOLOR_A(lua_State *L)
{
  int alpha = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) COLOR_A(alpha));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_COLOR_MASK        0x20000000UL // requires OR'd arguments
#define COLOR_MASK(r, g, b, a) ((32UL<<24)|(((r)&1UL)<<3)|(((g)&1UL)<<2)|(((b)&1UL)<<1)|(((a)&1UL)<<0))
static int LuaCOLOR_MASK(lua_State *L)
{
  int r = luaL_checkinteger(L, 1);
  int g = luaL_checkinteger(L, 2);
  int b = luaL_checkinteger(L, 3);
  int a = luaL_checkinteger(L, 4);
  lua_pushlightuserdata(L, (void*) COLOR_MASK(r, g, b, a));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_COLOR_RGB          0x04000000UL // requires OR'd arguments
#define COLOR_RGB(red, green, blue) ((4UL<<24)|(((red)&255UL)<<16)|(((green)&255UL)<<8)|(((blue)&255UL)<<0))
static int LuaCOLOR_RGB(lua_State *L)
{
  int red = luaL_checkinteger(L, 1);
  int green = luaL_checkinteger(L, 2);
  int blue = luaL_checkinteger(L, 3);
  lua_pushlightuserdata(L, (void*) COLOR_RGB(red, green, blue));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_CLEAR_RGB          0x02000000UL // requires OR'd arguments
#define CLEAR_RGB(red, green, blue) ((2UL<<24)|(((red)&255UL)<<16)|(((green)&255UL)<<8)|(((blue)&255UL)<<0))
static int LuaCLEAR_RGB(lua_State *L)
{
  int red = luaL_checkinteger(L, 1);
  int green = luaL_checkinteger(L, 2);
  int blue = luaL_checkinteger(L, 3);
  lua_pushlightuserdata(L, (void*) CLEAR_RGB(red, green, blue));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_DISPLAY            0x00000000UL
#define DISPLAY() ((0UL<<24))
static int LuaDISPLAY(lua_State *L)
{
  lua_pushlightuserdata(L, (void*) DISPLAY());
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_END                0x21000000UL
#define END() ((33UL<<24))
static int LuaEND(lua_State *L)
{
  lua_pushlightuserdata(L, (void*) END());
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_JUMP              0x1E000000UL // requires OR'd arguments
#define JUMP(dest) ((30UL<<24)|(((dest)&65535UL)<<0))
static int LuaJUMP(lua_State *L)
{
  int dest = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) JUMP(dest));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_LINE_WIDTH        0x0E000000UL // requires OR'd arguments
#define LINE_WIDTH(width) ((14UL<<24)|(((width)&4095UL)<<0))
static int LuaLINE_WIDTH(lua_State *L)
{
  int width = luaL_checknumber(L, 1) * 16.0;
  lua_pushlightuserdata(L, (void*) LINE_WIDTH(width));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_MACRO              0x25000000UL // requires OR'd arguments
#define MACRO(m) ((37UL<<24)|(((m)&1UL)<<0))
static int LuaMACRO(lua_State *L)
{
  int m = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) MACRO(m));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_POINT_SIZE        0x0D000000UL // requires OR'd arguments
#define POINT_SIZE(size) ((13UL<<24)|(((size)&8191UL)<<0))
static int LuaPOINT_SIZE(lua_State *L)
{
  int size = luaL_checknumber(L, 1) * 16.0;
  lua_pushlightuserdata(L, (void*) POINT_SIZE(size));
  return 1;
}
/*---------------------------------------------------------------------------*/
#ifdef HAS_RAW
  static int LuaRAW(lua_State *L)
  {
    int raw = luaL_checkinteger(L, 1);
    lua_pushlightuserdata(L, (void*) raw);
    return 1;
  }
#endif
/*---------------------------------------------------------------------------*/
// DL_RESTORE_CONTEXT    0x23000000UL
#define RESTORE_CONTEXT() ((35UL<<24))
static int LuaRESTORE_CONTEXT(lua_State *L)
{
  lua_pushlightuserdata(L, (void*) RESTORE_CONTEXT());
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_RETURN            0x24000000UL
#define RETURN() ((36UL<<24))
static int LuaRETURN(lua_State *L)
{
  lua_pushlightuserdata(L, (void*) RETURN());
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_SAVE_CONTEXT      0x22000000UL
#define SAVE_CONTEXT() ((34UL<<24))
static int LuaSAVE_CONTEXT(lua_State *L)
{
  lua_pushlightuserdata(L, (void*) SAVE_CONTEXT());
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_SCISSOR_SIZE      0x1C000000UL // requires OR'd arguments
#define SCISSOR_SIZE(width, height) ((28UL<<24)|(((width)&1023UL)<<10)|(((height)&1023UL)<<0))
static int LuaSCISSOR_SIZE(lua_State *L)
{
  int width = luaL_checkinteger(L, 1);
  int height = luaL_checkinteger(L, 2);
  lua_pushlightuserdata(L, (void*) SCISSOR_SIZE(width, height));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_SCISSOR_XY        0x1B000000UL // requires OR'd arguments
#define SCISSOR_XY(x, y) ((27UL<<24)|(((x)&511UL)<<9)|(((y)&511UL)<<0))
static int LuaSCISSOR_XY(lua_State *L)
{
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  lua_pushlightuserdata(L, (void*) SCISSOR_XY(x, y));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_STENCIL_FUNC      0x0A000000UL // requires OR'd arguments
#define STENCIL_FUNC(func, ref, mask) ((10UL<<24)|(((func)&7UL)<<16)|(((ref)&255UL)<<8)|(((mask)&255UL)<<0))
static int LuaSTENCIL_FUNC(lua_State *L)
{
  const static char* strings[] = {"NEVER", "LESS", "LEQUAL", "GREATER", "GEQUAL", "EQUAL", "NOTEQUAL", "ALWAYS", 0};
  int func = StringToNumber(luaL_checkstring(L, 1), strings, 7);
  int ref = luaL_checkinteger(L, 2);
  int mask = luaL_checkinteger(L, 3);
  lua_pushlightuserdata(L, (void*) STENCIL_FUNC(func, ref, mask));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_STENCIL_MASK      0x13000000UL // requires OR'd arguments
#define STENCIL_MASK(mask) ((19UL<<24)|(((mask)&255UL)<<0))
static int LuaSTENCIL_MASK(lua_State *L)
{
  int mask = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) STENCIL_MASK(mask));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_STENCIL_OP        0x0C000000UL // requires OR'd arguments
#define STENCIL_OP(sfail, spass) ((12UL<<24)|(((sfail)&7UL)<<3)|(((spass)&7UL)<<0))
static int LuaSTENCIL_OP(lua_State *L)
{
  const static char* strings[] = {"KEEP", "ZERO", "REPLACE", "INCR", "DECR", "INVERT", 0};
  int sfail = StringToNumber(luaL_checkstring(L, 1), strings, 2);
  int spass = StringToNumber(luaL_checkstring(L, 2), strings, 2);
  lua_pushlightuserdata(L, (void*) STENCIL_OP(sfail, spass));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_TAG               0x03000000UL // requires OR'd arguments
#define TAG(t) ((3UL<<24)|(((t)&255UL)<<0))
static int LuaTAG(lua_State *L)
{
  int t = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) TAG(t));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_TAG_MASK          0x14000000UL // requires OR'd arguments
#define TAG_MASK(mask) ((20UL<<24)|(((mask)&1UL)<<0))
static int LuaTAG_MASK(lua_State *L)
{
  int mask = luaL_checkinteger(L, 1);
  lua_pushlightuserdata(L, (void*) TAG_MASK(mask));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_VERTEX2F          0x40000000UL // requires OR'd arguments
#define VERTEX2F(x, y) ((1UL<<30)|(((x)&32767UL)<<15)|(((y)&32767UL)<<0))
static int LuaVERTEX2F(lua_State *L)
{
  int x = luaL_checknumber(L, 1) * 16.0;
  int y = luaL_checknumber(L, 2) * 16.0;
  lua_pushlightuserdata(L, (void*) VERTEX2F(x, y));
  return 1;
}
/*---------------------------------------------------------------------------*/
// DL_VERTEX2II          0x02000000UL // requires OR'd arguments
#define VERTEX2II(x, y, handle, cell) ((2UL<<30)|(((x)&511UL)<<21)|(((y)&511UL)<<12)|(((handle)&31UL)<<7)|(((cell)&127UL)<<0))
static int LuaVERTEX2II(lua_State *L)
{
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int handle = luaL_checkinteger(L, 3);
  int cell = luaL_checkinteger(L, 4);
  lua_pushlightuserdata(L, (void*) VERTEX2II(x, y, handle, cell));
  return 1;
}
/*---------------------------------------------------------------------------*/
// Lua co-processor functions
/*---------------------------------------------------------------------------*/
// CMD_TEXT
static int LuaCMD_TEXT(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 5, "x, y, font, options, s");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t font = luaL_checkinteger(L, 3);
  uint16_t options = luaL_checkinteger(L, 4);
  const char* s = luaL_checkstring(L, 5);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_TEXT));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | (uint32_t) font)));
  lua_rawseti(L, -2, 3);
  lua_pushstring(L, (s));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_NUMBER
static int LuaCMD_NUMBER(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 5, "x, y, font, options, n");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t font = luaL_checkinteger(L, 3);
  uint16_t options = luaL_checkinteger(L, 4);
  int32_t n = luaL_checkinteger(L, 5);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_NUMBER));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | font)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (n));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_LOADIDENTITY
static int LuaCMD_LOADIDENTITY(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_LOADIDENTITY));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_TOGGLE
static int LuaCMD_TOGGLE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 7, "x, y, w, font, options, state, s");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t font = luaL_checkinteger(L, 4);
  uint16_t options = luaL_checkinteger(L, 5);
  uint16_t state = luaL_checkinteger(L, 6);
  const char* s = luaL_checkstring(L, 7);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_TOGGLE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) font << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) state << 16) | options)));
  lua_rawseti(L, -2, 4);
  lua_pushstring(L, (s));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GAUGE
static int LuaCMD_GAUGE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 8, "x, y, r, options, major, minor, val, range");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t r = luaL_checkinteger(L, 3);
  uint16_t options = luaL_checkinteger(L, 4);
  uint16_t major = luaL_checkinteger(L, 5);
  uint16_t minor = luaL_checkinteger(L, 6);
  uint16_t val = luaL_checkinteger(L, 7);
  uint16_t range = luaL_checkinteger(L, 8);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GAUGE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | r)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) minor << 16) | major)));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) range << 16) | val)));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_REGREAD
static int LuaCMD_REGREAD(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "ptr, result");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t result = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_REGREAD));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (0));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GETPROPS
static int LuaCMD_GETPROPS(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 3, "ptr, w, h");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t w = luaL_checkinteger(L, 2);
  uint32_t h = luaL_checkinteger(L, 3);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GETPROPS));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (w));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (h));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_MEMCPY
static int LuaCMD_MEMCPY(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 3, "dest, src, num");
  uint32_t dest = luaL_checkinteger(L, 1);
  uint32_t src = luaL_checkinteger(L, 2);
  uint32_t num = luaL_checkinteger(L, 3);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_MEMCPY));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (dest));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (src));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SPINNER
static int LuaCMD_SPINNER(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 4, ".x,y,style,scale");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  uint16_t style = luaL_checkinteger(L, 3);
  uint16_t scale = luaL_checkinteger(L, 4);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SPINNER));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) scale << 16) | style)));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_BGCOLOR
static int LuaCMD_BGCOLOR(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 3, ".r,g,b");
  uint8_t r = luaL_checkinteger(L, 1);
  uint8_t g = luaL_checkinteger(L, 2);
  uint8_t b = luaL_checkinteger(L, 3);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_BGCOLOR));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((r << 16) | (g << 8) | b));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SWAP
static int LuaCMD_SWAP(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_SWAP));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_INFLATE
static int LuaCMD_INFLATE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "ptr");
  uint32_t ptr = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_INFLATE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_TRANSLATE
static int LuaCMD_TRANSLATE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "tx, ty");
  int32_t tx = luaL_checkinteger(L, 1);
  int32_t ty = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_TRANSLATE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (tx));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (ty));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_STOP
static int LuaCMD_STOP(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_STOP));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SLIDER
static int LuaCMD_SLIDER(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 7, "x, y, w, h, options, val, range");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  uint16_t options = luaL_checkinteger(L, 5);
  uint16_t val = luaL_checkinteger(L, 6);
  uint16_t range = luaL_checkinteger(L, 7);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SLIDER));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) val << 16) | options)));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (uint32_t) (range));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_TOUCHTRANSFORM
static int LuaCMD_TOUCHTRANSFORM(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 13, "x0, y0, x1, y1, x2, y2, tx0, ty0, tx1, ty1, tx2, ty2, result");
  int32_t x0 = luaL_checkinteger(L, 1);
  int32_t y0 = luaL_checkinteger(L, 2);
  int32_t x1 = luaL_checkinteger(L, 3);
  int32_t y1 = luaL_checkinteger(L, 4);
  int32_t x2 = luaL_checkinteger(L, 5);
  int32_t y2 = luaL_checkinteger(L, 6);
  int32_t tx0 = luaL_checkinteger(L, 7);
  int32_t ty0 = luaL_checkinteger(L, 8);
  int32_t tx1 = luaL_checkinteger(L, 9);
  int32_t ty1 = luaL_checkinteger(L, 10);
  int32_t tx2 = luaL_checkinteger(L, 11);
  int32_t ty2 = luaL_checkinteger(L, 12);
  uint16_t result = luaL_checkinteger(L, 13);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_TOUCH_TRANSFORM));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (x0));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (y0));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (x1));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (y1));
  lua_rawseti(L, -2, 5);
  lua_pushlightuserdata(L, (void*) (x2));
  lua_rawseti(L, -2, 6);
  lua_pushlightuserdata(L, (void*) (y2));
  lua_rawseti(L, -2, 7);
  lua_pushlightuserdata(L, (void*) (tx0));
  lua_rawseti(L, -2, 8);
  lua_pushlightuserdata(L, (void*) (ty0));
  lua_rawseti(L, -2, 9);
  lua_pushlightuserdata(L, (void*) (tx1));
  lua_rawseti(L, -2, 10);
  lua_pushlightuserdata(L, (void*) (ty1));
  lua_rawseti(L, -2, 11);
  lua_pushlightuserdata(L, (void*) (tx2));
  lua_rawseti(L, -2, 12);
  lua_pushlightuserdata(L, (void*) (ty2));
  lua_rawseti(L, -2, 13);
  lua_pushlightuserdata(L, (void*) (uint32_t) (result));
  lua_rawseti(L, -2, 14);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_INTERRUPT
static int LuaCMD_INTERRUPT(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "ms");
  uint32_t ms = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_INTERRUPT));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ms));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_FGCOLOR
static int LuaCMD_FGCOLOR(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "c");
  uint32_t c = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_FGCOLOR));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (c));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_ROTATE
static int LuaCMD_ROTATE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "a");
  int32_t a = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_ROTATE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (a));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_BUTTON
static int LuaCMD_BUTTON(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 7, "x,y,w,h,font,options,s");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  int16_t font = luaL_checkinteger(L, 5);
  uint16_t options = luaL_checkinteger(L, 6);
  const char* s = luaL_checkstring(L, 7);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_BUTTON));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | x)));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | font)));
  lua_rawseti(L, -2, 4);
  lua_pushstring(L, (s));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_MEMWRITE
static int LuaCMD_MEMWRITE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "ptr, num");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t num = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_MEMWRITE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SCROLLBAR
static int LuaCMD_SCROLLBAR(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 8, "x, y, w, h, options, val, size, range");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  uint16_t options = luaL_checkinteger(L, 5);
  uint16_t val = luaL_checkinteger(L, 6);
  uint16_t size = luaL_checkinteger(L, 7);
  uint16_t range = luaL_checkinteger(L, 8);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SCROLLBAR));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) val << 16) | options)));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) range << 16) | size)));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GETMATRIX
static int LuaCMD_GETMATRIX(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 6, "a, b, c, d, e, f");
  int32_t a = luaL_checkinteger(L, 1);
  int32_t b = luaL_checkinteger(L, 2);
  int32_t c = luaL_checkinteger(L, 3);
  int32_t d = luaL_checkinteger(L, 4);
  int32_t e = luaL_checkinteger(L, 5);
  int32_t f = luaL_checkinteger(L, 6);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GETMATRIX));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (a));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (b));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (c));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (d));
  lua_rawseti(L, -2, 5);
  lua_pushlightuserdata(L, (void*) (e));
  lua_rawseti(L, -2, 6);
  lua_pushlightuserdata(L, (void*) (f));
  lua_rawseti(L, -2, 7);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SKETCH
static int LuaCMD_SKETCH(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 6, "x, y, w, h, ptr, format");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  uint16_t w = luaL_checkinteger(L, 3);
  uint16_t h = luaL_checkinteger(L, 4);
  uint32_t ptr = luaL_checkinteger(L, 5);
  uint16_t format = luaL_checkinteger(L, 6);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SKETCH));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (uint32_t) (format));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_MEMSET
static int LuaCMD_MEMSET(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 3, "ptr, value, num");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t value = luaL_checkinteger(L, 2);
  uint32_t num = luaL_checkinteger(L, 3);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_MEMSET));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (value));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GRADCOLOR
static int LuaCMD_GRADCOLOR(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "c");
  uint32_t c = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GRADCOLOR));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (c));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_BITMAPTRANSFORM
static int LuaCMD_BITMAPTRANSFORM(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 13, "x0, y0, x1, y1, x2, y2, tx0, ty0, tx1, ty1, tx2, ty2, result");
  int32_t x0 = luaL_checkinteger(L, 1);
  int32_t y0 = luaL_checkinteger(L, 2);
  int32_t x1 = luaL_checkinteger(L, 3);
  int32_t y1 = luaL_checkinteger(L, 4);
  int32_t x2 = luaL_checkinteger(L, 5);
  int32_t y2 = luaL_checkinteger(L, 6);
  int32_t tx0 = luaL_checkinteger(L, 7);
  int32_t ty0 = luaL_checkinteger(L, 8);
  int32_t tx1 = luaL_checkinteger(L, 9);
  int32_t ty1 = luaL_checkinteger(L, 10);
  int32_t tx2 = luaL_checkinteger(L, 11);
  int32_t ty2 = luaL_checkinteger(L, 12);
  uint16_t result = luaL_checkinteger(L, 13);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_BITMAP_TRANSFORM));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (x0));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (y0));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (x1));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (y1));
  lua_rawseti(L, -2, 5);
  lua_pushlightuserdata(L, (void*) (x2));
  lua_rawseti(L, -2, 6);
  lua_pushlightuserdata(L, (void*) (y2));
  lua_rawseti(L, -2, 7);
  lua_pushlightuserdata(L, (void*) (tx0));
  lua_rawseti(L, -2, 8);
  lua_pushlightuserdata(L, (void*) (ty0));
  lua_rawseti(L, -2, 9);
  lua_pushlightuserdata(L, (void*) (tx1));
  lua_rawseti(L, -2, 10);
  lua_pushlightuserdata(L, (void*) (ty1));
  lua_rawseti(L, -2, 11);
  lua_pushlightuserdata(L, (void*) (tx2));
  lua_rawseti(L, -2, 12);
  lua_pushlightuserdata(L, (void*) (ty2));
  lua_rawseti(L, -2, 13);
  lua_pushlightuserdata(L, (void*) (uint32_t) (result));
  lua_rawseti(L, -2, 14);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_CALIBRATE
static int LuaCMD_CALIBRATE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "result");
  uint32_t result = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_CALIBRATE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (result));
  lua_rawseti(L, -2, 2);
  //Ft_Gpu_Hal_WaitCmdfifo_empty());
  //lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SETFONT
static int LuaCMD_SETFONT(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "font, ptr");
  uint32_t font = luaL_checkinteger(L, 1);
  uint32_t ptr = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SETFONT));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (font));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_LOGO
static int LuaCMD_LOGO(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_LOGO));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_APPEND
static int LuaCMD_APPEND(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "ptr, num");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t num = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_APPEND));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_MEMZERO
static int LuaCMD_MEMZERO(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "ptr, num");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t num = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_MEMZERO));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SCALE
static int LuaCMD_SCALE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "sx, sy");
  int32_t sx = luaL_checkinteger(L, 1);
  int32_t sy = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SCALE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (sx));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (sy));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_CLOCK
static int LuaCMD_CLOCK(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 8, "x, y, r, options, h, m, s, ms");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t r = luaL_checkinteger(L, 3);
  uint16_t options = luaL_checkinteger(L, 4);
  uint16_t h = luaL_checkinteger(L, 5);
  uint16_t m = luaL_checkinteger(L, 6);
  uint16_t s = luaL_checkinteger(L, 7);
  uint16_t ms = luaL_checkinteger(L, 8);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_CLOCK));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | r)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) m << 16) | h)));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) ms << 16) | s)));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GRADIENT
static int LuaCMD_GRADIENT(lua_State *L)
{
  uint32_t RGB(int i)
  {
    return ((luaL_checkinteger(L, i) & 0xFF) << 16) | ((luaL_checkinteger(L, i + 1) & 0xFF) << 8) | (luaL_checkinteger(L, i + 2) & 0xFF);
  }

  LuaUtils_CheckParameter1(L, 10, "x0,y0,r0,g0,b0,x1,y1,r1,g1,b1");
  int16_t x0 = luaL_checkinteger(L, 1);
  int16_t y0 = luaL_checkinteger(L, 2);
  uint32_t rgb0 = RGB(3);
  int16_t x1 = luaL_checkinteger(L, 6);
  int16_t y1 = luaL_checkinteger(L, 7);
  uint32_t rgb1 = RGB(8);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GRADIENT));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y0 << 16) | (x0 & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (rgb0));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y1 << 16) | (x1 & 0xffff))));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (rgb1));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SETMATRIX
static int LuaCMD_SETMATRIX(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_SETMATRIX));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_TRACK
static int LuaCMD_TRACK(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 5, "x, y, w, h, tag");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  int16_t tag = luaL_checkinteger(L, 5);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_TRACK));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (uint32_t) (tag));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_GETPTR
static int LuaCMD_GETPTR(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "result");
  uint32_t result = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_GETPTR));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (result));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_PROGRESS
static int LuaCMD_PROGRESS(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 7, "x, y, w, h, options, val, range");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  uint16_t options = luaL_checkinteger(L, 5);
  uint16_t val = luaL_checkinteger(L, 6);
  uint16_t range = luaL_checkinteger(L, 7);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_PROGRESS));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) val << 16) | options)));
  lua_rawseti(L, -2, 4);
  lua_pushlightuserdata(L, (void*) (uint32_t) (range));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_COLDSTART
static int LuaCMD_COLDSTART(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_COLDSTART));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_KEYS
static int LuaCMD_KEYS(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 7, "x, y, w, h, font, options, s");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t w = luaL_checkinteger(L, 3);
  int16_t h = luaL_checkinteger(L, 4);
  int16_t font = luaL_checkinteger(L, 5);
  uint16_t options = luaL_checkinteger(L, 6);
  const char* s = luaL_checkstring(L, 7);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_KEYS));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) h << 16) | w)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | font)));
  lua_rawseti(L, -2, 4);
  lua_pushstring(L, (s));
  lua_rawseti(L, -2, 5);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_DIAL
static int LuaCMD_DIAL(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 5, "x, y, r, options, val");
  int16_t x = luaL_checkinteger(L, 1);
  int16_t y = luaL_checkinteger(L, 2);
  int16_t r = luaL_checkinteger(L, 3);
  uint16_t options = luaL_checkinteger(L, 4);
  uint16_t val = luaL_checkinteger(L, 5);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_DIAL));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) y << 16) | (x & 0xffff))));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) ((((uint32_t) options << 16) | r)));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (uint32_t) (val));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_LOADIMAGE
static int LuaCMD_LOADIMAGE(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 2, "ptr, options");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t options = luaL_checkinteger(L, 2);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_LOADIMAGE));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (options));
  lua_rawseti(L, -2, 3);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_DLSTART
static int LuaCMD_DLSTART(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_DLSTART));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SNAPSHOT
static int LuaCMD_SNAPSHOT(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 1, "ptr");
  uint32_t ptr = luaL_checkinteger(L, 1);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_SNAPSHOT));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_SCREENSAVER
static int LuaCMD_SCREENSAVER(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 0, 0);
  lua_pushlightuserdata(L, (void*) (CMD_SCREENSAVER));
  return 1;
}
/*---------------------------------------------------------------------------*/
// CMD_MEMCRC
static int LuaCMD_MEMCRC(lua_State *L)
{
  LuaUtils_CheckParameter1(L, 3, "ptr, num, result");
  uint32_t ptr = luaL_checkinteger(L, 1);
  uint32_t num = luaL_checkinteger(L, 2);
  uint32_t result = luaL_checkinteger(L, 3);
  lua_newtable(L);
  lua_pushlightuserdata(L, (void*) (CMD_MEMCRC));
  lua_rawseti(L, -2, 1);
  lua_pushlightuserdata(L, (void*) (ptr));
  lua_rawseti(L, -2, 2);
  lua_pushlightuserdata(L, (void*) (num));
  lua_rawseti(L, -2, 3);
  lua_pushlightuserdata(L, (void*) (result));
  lua_rawseti(L, -2, 4);
  return 1;
}
/*---------------------------------------------------------------------------*/
// Lua basics
/*---------------------------------------------------------------------------*/
static const luaL_Reg Lib2[] = {
  // Display list
  {"ALPHA_FUNC",          LuaALPHA_FUNC},
  {"BITMAP_HANDLE",       LuaBITMAP_HANDLE},
  {"BITMAP_LAYOUT",       LuaBITMAP_LAYOUT},
  {"BITMAP_SIZE",         LuaBITMAP_SIZE},
  {"BITMAP_SOURCE",       LuaBITMAP_SOURCE},
  {"BITMAP_TFORM_A",      LuaBITMAP_TFORM_A},
  {"BITMAP_TFORM_B",      LuaBITMAP_TFORM_B},
  {"BITMAP_TFORM_C",      LuaBITMAP_TFORM_C},
  {"BITMAP_TFORM_D",      LuaBITMAP_TFORM_D},
  {"BITMAP_TFORM_E",      LuaBITMAP_TFORM_E},
  {"BITMAP_TFORM_F",      LuaBITMAP_TFORM_F},
  {"BLEND_FUNC",          LuaBLEND_FUNC},
  {"BEGIN",               LuaBEGIN},
  {"CALL",                LuaCALL},
  {"CLEAR",               LuaCLEAR},
  {"CELL",                LuaCELL},
  {"CLEAR_RGB",           LuaCLEAR_RGB},
  {"CLEAR_STENCIL",       LuaCLEAR_STENCIL},
  {"CLEAR_TAG",           LuaCLEAR_TAG},
  {"COLOR_A",             LuaCOLOR_A},
  {"COLOR_MASK",          LuaCOLOR_MASK},
  {"COLOR_RGB",           LuaCOLOR_RGB},
  {"DISPLAY",             LuaDISPLAY},
  {"END",                 LuaEND},
  {"JUMP",                LuaJUMP},
  {"LINE_WIDTH",          LuaLINE_WIDTH},
  {"MACRO",               LuaMACRO},
  {"POINT_SIZE",          LuaPOINT_SIZE},
  #ifdef HAS_RAW
    {"RAW",                 LuaRAW},
  #endif
  {"RESTORE_CONTEXT",     LuaRESTORE_CONTEXT},
  {"RETURN",              LuaRETURN},
  {"SAVE_CONTEXT",        LuaSAVE_CONTEXT},
  {"SCISSOR_SIZE",        LuaSCISSOR_SIZE},
  {"SCISSOR_XY",          LuaSCISSOR_XY},
  {"STENCIL_FUNC",        LuaSTENCIL_FUNC},
  {"STENCIL_MASK",        LuaSTENCIL_MASK},
  {"STENCIL_OP",          LuaSTENCIL_OP},
  {"TAG",                 LuaTAG},
  {"TAG_MASK",            LuaTAG_MASK},
  {"VERTEX2F",            LuaVERTEX2F},
  {"VERTEX2II",           LuaVERTEX2II},
  // Coprocessor
  {"CMD_TEXT",            LuaCMD_TEXT},
  {"CMD_NUMBER",          LuaCMD_NUMBER},
  {"CMD_LOADIDENTITY",    LuaCMD_LOADIDENTITY},
  {"CMD_TOGGLE",          LuaCMD_TOGGLE},
  {"CMD_GAUGE",           LuaCMD_GAUGE},
  {"CMD_REGREAD",         LuaCMD_REGREAD},
  {"CMD_GETPROPS",        LuaCMD_GETPROPS},
  {"CMD_MEMCPY",          LuaCMD_MEMCPY},
  {"CMD_SPINNER",         LuaCMD_SPINNER},
  {"CMD_BGCOLOR",         LuaCMD_BGCOLOR},
  {"CMD_SWAP",            LuaCMD_SWAP},
  {"CMD_INFLATE",         LuaCMD_INFLATE},
  {"CMD_TRANSLATE",       LuaCMD_TRANSLATE},
  {"CMD_STOP",            LuaCMD_STOP},
  {"CMD_SLIDER",          LuaCMD_SLIDER},
  {"CMD_TOUCHTRANSFORM",  LuaCMD_TOUCHTRANSFORM},
  {"CMD_INTERRUPT",       LuaCMD_INTERRUPT},
  {"CMD_FGCOLOR",         LuaCMD_FGCOLOR},
  {"CMD_ROTATE",          LuaCMD_ROTATE},
  {"CMD_BUTTON",          LuaCMD_BUTTON},
  {"CMD_MEMWRITE",        LuaCMD_MEMWRITE},
  {"CMD_SCROLLBAR",       LuaCMD_SCROLLBAR},
  {"CMD_GETMATRIX",       LuaCMD_GETMATRIX},
  {"CMD_SKETCH",          LuaCMD_SKETCH},
  {"CMD_MEMSET",          LuaCMD_MEMSET},
  {"CMD_GRADCOLOR",       LuaCMD_GRADCOLOR},
  {"CMD_BITMAPTRANSFORM", LuaCMD_BITMAPTRANSFORM},
  {"CMD_CALIBRATE",       LuaCMD_CALIBRATE},
  {"CMD_SETFONT",         LuaCMD_SETFONT},
  {"CMD_LOGO",            LuaCMD_LOGO},
  {"CMD_APPEND",          LuaCMD_APPEND},
  {"CMD_MEMZERO",         LuaCMD_MEMZERO},
  {"CMD_SCALE",           LuaCMD_SCALE},
  {"CMD_CLOCK",           LuaCMD_CLOCK},
  {"CMD_GRADIENT",        LuaCMD_GRADIENT},
  {"CMD_SETMATRIX",       LuaCMD_SETMATRIX},
  {"CMD_TRACK",           LuaCMD_TRACK},
  {"CMD_GETPTR",          LuaCMD_GETPTR},
  {"CMD_PROGRESS",        LuaCMD_PROGRESS},
  {"CMD_COLDSTART",       LuaCMD_COLDSTART},
  {"CMD_KEYS",            LuaCMD_KEYS},
  {"CMD_DIAL",            LuaCMD_DIAL},
  {"CMD_LOADIMAGE",       LuaCMD_LOADIMAGE},
  {"CMD_DLSTART",         LuaCMD_DLSTART},
  {"CMD_SNAPSHOT",        LuaCMD_SNAPSHOT},
  {"CMD_SCREENSAVER",     LuaCMD_SCREENSAVER},
  {"CMD_MEMCRC",          LuaCMD_MEMCRC},
  {0, 0}
};
/*---------------------------------------------------------------------------*/
/*!
 * \brief Open EVE driver
 * \return A table with all display list and co-processor functions. Every
 * of this function returns a light user data value or an array of them.
 * This values can be used in the command list for the EVE chip.
 */
LIB_FUNCTION(LuaOpen, "commands|E.C")
{
  LuaUtils_CheckParameter1(L, 1, LuaOpenUsage);
  int channel = luaL_checknumber(L, 1);
  int16_t result = EVE_Open(channel);
  if (result < 0)
    return LuaUtils_Error(L, HAL_ErrorToString(result));
  int count = 0;
  for (const luaL_Reg *lib = Lib2; lib->name; lib++) count++;
  lua_createtable(L, 0, count);
  for (const luaL_Reg *lib = Lib2; lib->name; lib++) {
    lua_pushstring(L, lib->name);
    lua_pushcfunction(L, lib->func);
    lua_settable(L, -3);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Close the EVE driver.
 * \note Free your commandFunctions list by setting it to nil.
 */
LIB_FUNCTION(LuaClose, ".")
{
  LuaUtils_CheckParameter1(L, 0, LuaCloseUsage);
  EVE_Close();
  return 0;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Set a command list
 * A command list is an array of light user data, strings, function or
 * command list.
 * - Light user data will be transfered directly to EVEs Graphic Engine
 *   Command buffer.
 * - String, a string is splitted in four chars, at the end of the string
 *   padded with 0, and send to the command buffer.
 * - Function. The function is called. The result can be a command list,
 *   any count of user data. The list will be invokted, the user data
 *   are send to the command buffer.
 * - Command list will be invoked.
 */
LIB_FUNCTION(LuaSet, "true|E.displayList")
{
  // The context, helpful if an error occured
  struct {
    uint16_t index, level;
  } context;

  int16_t Append(uint32_t item)
  {
    //DEBUG("%d %d %08X\n", context.index, context.level, item);
    return EVE_CommandAppend(item);
  }

  int16_t AppendString(char *s)
  {
    //DEBUG("%d %d %s\n", context.index, context.level, s);
    return EVE_CommandAppendString(s);
  }

  // Invoke the command list
  int16_t IlterateTable(int level)
  {
    context.level = level;                                // Remember the level
    luaL_checktype(L, -1, LUA_TTABLE);                    // Top element must be the table
    for (uint16_t index = 1; true; index++) {             // Walk to all elements
      context.index = index;                              // Rember the array index
      lua_rawgeti(L, -1, index);                          // Get element index from the table
      int type = lua_type(L, -1);                         // Get the element type
      switch (type) {

        case LUA_TNIL :
          lua_pop(L, 1);                                 // Pop the element from rawgeti()
          return RESULT_OK;

        case LUA_TLIGHTUSERDATA :
          {
            uint32_t item = (uint32_t) lua_touserdata(L, -1);
            int16_t result = Append(item);
            if (result < 0)
              return result;
            lua_pop(L, 1);                                 // Pop the element from rawgeti()
          }
          break;

        case LUA_TSTRING :
          {
            const char *s = lua_tostring(L, -1);
            int16_t result = AppendString(s);
            if (result < 0)
              return result;
            lua_pop(L, 1);                                 // Pop the element from rawgeti()
          }
          break;

        case LUA_TTABLE :
          IlterateTable(level + 1);                        // Invoke the table
          lua_pop(L, 1);                                   // Pop the element from rawgeti()
          break;

        case LUA_TFUNCTION :
          {
            int top = lua_gettop(L);                       // Top element is the function, then the next is the table
            if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) // Call the function. This pops also the function!
              luaL_error(L, "Error during display list function call at %d, nesting level %d:\n\t%s", index, level, lua_tostring(L, -1));
            int n = lua_gettop(L) - top + 1;               // Get the number of return elements. Add one to correct the poped function.
            if (n) {                                       // Function returns items
              if (lua_type(L, -1) == LUA_TTABLE)
                IlterateTable(level + 1);                  // Invoke the table and ignore other returned elements
              else
                for (int i = -n; i <= -1; i++) {
                  uint32_t item = (uint32_t) lua_touserdata(L, i);
                  int16_t result = Append(item);
                  if (result < 0)
                    return result;
                }
              lua_pop(L, n);                               // Pop all returned elements
            }
          }
          break;

        case LUA_TTHREAD :
          // TODO
          break;

        default :
          return RESULT_LIBEVE_ERROR_WRONG_DATATYPE;

      }

    }
  }

  memset(&context, 0, sizeof(context));
  LuaUtils_CheckParameter1(L, 1, LuaSetUsage);
  int16_t result = EVE_CommandBegin();
  if (result >= RESULT_OK)
    result = IlterateTable(1);
  if (result >= RESULT_OK)
    result = EVE_CommandEnd();
  if (result < 0) {
    switch (result) {
      case RESULT_EVE_ERROR_COPROCESSOR :
        return LuaUtils_Error(L, "EVE: Coprocessor error at index %d, level %d", context.index, context.level);
      case RESULT_LIBEVE_ERROR_WRONG_DATATYPE :
        return LuaUtils_Error(L, "EVE: Wrong datatype at index %d, level %d", context.index, context.level);
     }
     return LuaUtils_Error(L, "EVE: Unknown error %d at index %d, level %d", result, context.index, context.level);
  }
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Load a raw ot bitmap image 
 */
LIB_FUNCTION(LuaLoad, "address,size|E.address,N{,type}")
{
  FIL f;
  uint8_t *image = 0;
  uint32_t size;

  void Cleanup(void)
  {
    Memory_Free(image, size);
    f_close(&f);
  }

  int n = LuaUtils_CheckParameter2(L, 2, 3, LuaLoadUsage);
  uint32_t address = luaL_checkinteger(L, 1);
  const char *name = luaL_checkstring(L, 2);
  const char *type = strrchr(name, '.');
  if (type)
    type++;
  if (n >= 3)
    type = luaL_checkstring(L, 3);
  enum {tiRAW, tiBMP} typeId;
  if (strcasecmp(type, "raw") == 0)
    typeId = tiRAW;
  else if (strcasecmp(type, "bmp") == 0)
    typeId = tiBMP;
  else
    return LuaUtils_Error(L, "Unknown picture type");
  FRESULT result = f_open(&f, name, FA_READ);
  if (result != FR_OK)
    return LuaUtils_Error(L, "Can't open %s", name);
  size = f_size(&f);
  image = Memory_Allocate(size, 0);
  if (!image)
    return Cleanup(), LuaUtils_Error(L, "Out of memory");
  uint32_t readSize;
  result = f_read(&f, image, size, &readSize);
  if (result != FR_OK)
    return Cleanup(), LuaUtils_Error(L, "Can't open %s", name);
  if (typeId = tiRAW) {
    EVE_WriteMain(0, image, size);
    lua_pushinteger(L, address + size);
    lua_pushinteger(L, size);
    return Cleanup(), 2;
  }
  if (typeId = tiBMP) {
    typedef struct BMP {
      // Header
      uint16_t type;
      uint32_t fileSize;        // Could be incorrect, dont use!
      uint32_t reserved;
      uint32_t offsetBitmap;
      // Bitmap info
      uint32_t size;            // Size of struct from here
      int32_t width;
      int32_t height;
      uint16_t planes;
      uint16_t bitCount;
      uint32_t compression;
      uint32_t sizeImage;
      int32_t xPelsPerMeter;    // Pixels per meter x
      int32_t yPelsPerMeter;    // Pixels per meter y
      uint32_t colorsUsed;      // Colors used by the bitmap
      uint32_t colorsImportant; // Colors that are important
    } __attribute__ ((packed)) BMP;
    BMP  *bmp = (BMP*) image;
    if (bmp->type != 0x4D42)
      return Cleanup(), LuaUtils_Error(L, "Not a bmp file");
    if (bmp->compression)
      return Cleanup(), LuaUtils_Error(L, "Not supported bmp file");
    EVE_WriteMain(0, &image[bmp->offsetBitmap], bmp->sizeImage);
    lua_pushinteger(L, address + bmp->sizeImage);
    lua_pushinteger(L, size);
    return Cleanup(), 2;
  }
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "eve")
  LIB_ITEM_FUNCTION("open", LuaOpen)
  LIB_ITEM_FUNCTION("close", LuaClose)
  LIB_ITEM_FUNCTION("set", LuaSet)
  LIB_ITEM_FUNCTION("load", LuaLoad)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibEVE_Register(lua_State *L)
{
  LuaUtils_Register(L, "sys", "eve", Lib);
}
/*---------------------------------------------------------------------------*/

