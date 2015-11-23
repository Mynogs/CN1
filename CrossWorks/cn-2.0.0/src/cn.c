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
#include <string.h>
#include "Common.h"
#include "cn.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "../FatFs-0-10-c/src/ff.h"
#include "../HAL/Hal.h"
#include "../Memory.h"
#include "../LuaLibs/LuaLibW5500.h"
#include "../SysLog.h"
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
#define LINE_COUNT 128
/*---------------------------------------------------------------------------*/
/*!
 * \brief The three base librarys for NOGS
 */
void __attribute__ ((weak)) LuaLibLevelSys_Register(lua_State *L) {}
void __attribute__ ((weak)) LuaLibLevelHAL_Register(lua_State *L) {}
void __attribute__ ((weak)) LuaLibLevelApp_Register(lua_State *L) {}
/*---------------------------------------------------------------------------*/
//
/*---------------------------------------------------------------------------*/
#define TRACE_ITEMS 10
/*---------------------------------------------------------------------------*/
#if TRACE_ITEMS > 0
  typedef struct TraceItem {
    uint16_t line;
    char source[14];
  } __attribute__ ((packed)) TraceItem;
#endif
/*---------------------------------------------------------------------------*/
struct {
  lua_State *L;
  #if TRACE_ITEMS > 0
    TraceItem trace[TRACE_ITEMS];
  #endif
  char *throwError;
} cn;
/*---------------------------------------------------------------------------*/
//
/*---------------------------------------------------------------------------*/
/*static*/ void CN_ServeTick(lua_State *L)
{
/*
  if (HAL_TickIsTicked() && L) {
    double elapsedTime = HAL_TickGetElapsedTime();
    lua_getglobal(L, "sys");
    if (lua_istable(L, -1)) {
      lua_getfield(L, -1, "onTick");
      if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, elapsedTime);
        lua_sethook(L, 0, 0, 0); // Preven recrusive calls of the hook function
        lua_call(L, 1, 0);
      } else
        lua_remove(L, -1);
    } else
      lua_remove(L, -1);
  }
*/
/*
  if (HAL_TickIsTicked() && L) {
    double elapsedTime = HAL_TickGetElapsedTime();
    lua_getglobal(L, "sys");
    if (lua_istable(L, -1)) {
      lua_getfield(L, -1, "onTick");
      if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, elapsedTime);
        lua_sethook(L, 0, 0, 0); // Preven recrusive calls of the hook function
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
          luaL_error(L, "Error during 'onTick':\n\t%s", lua_tostring(L, -1));
      } else {
        #ifdef LUA53
          lua_remove(L, -1);
        #endif
      }
    }
    #ifdef LUA53
      lua_remove(L, -1);
    #endif
  }
*/
}
/*---------------------------------------------------------------------------*/
// The Lua hook function
/*---------------------------------------------------------------------------*/
static void luaHook(lua_State *L, lua_Debug *ar)
{
  if (cn.throwError)
    luaL_error(L, cn.throwError);
  if (ar->currentline >= 0) {
    #if TRACE_ITEMS > 0
      lua_getinfo(L, "S", ar);
      if (cn.trace[0].line != ar->currentline) {
        cn.trace[0].line = ar->currentline;
        if (ar->source[0] == '@')
          memcpy(cn.trace[0].source, ar->source + 1, 14);
        else
          memcpy(cn.trace[0].source, ar->source, 14);
        memmove(&cn.trace[1], &cn.trace[0], sizeof(cn.trace) - sizeof(cn.trace[0]));
      }
    #endif
  } else {
      //LuaLibW5500_Serve(L); // TODO besser
      //CN_ServeTick(L);
      //lua_sethook(cn.L, luaHook, LUA_MASKLINE | LUA_MASKCOUNT, LINE_COUNT);
  }
}
/*---------------------------------------------------------------------------*/
//
/*---------------------------------------------------------------------------*/
void CN_Init(void)
{
  memset(&cn, 0, sizeof(cn));
}
/*---------------------------------------------------------------------------*/
void CN_Dump(void)
{
  #if TRACE_ITEMS > 0
    printf("CN: Lua trace back:");
    char name[15];
    name[0] = 0;
    name[14] = 0;
    for (uint8_t i = 0; i < TRACE_ITEMS; i++) {
      if (!cn.trace[i].source[0] && !cn.trace[i].line) 
        break;
      if (memcmp(name, cn.trace[i].source, 14) == 0) 
        printf(" %u", cn.trace[i].line);
      else {
        memcpy(name, cn.trace[i].source, 14);
        printf("\nCN: %s:%u", name, cn.trace[i].line);
      }
    }
    printf("\n");
  #endif
}
/*---------------------------------------------------------------------------*/
void CN_Stop(void)
{
  cn.throwError = "stop";
}
/*---------------------------------------------------------------------------*/
void CN_Restart(void)
{
  cn.throwError = "restart";
}
/*---------------------------------------------------------------------------*/
void CN_Run(const char *path)
{
  bool restart;
  do {
    restart = false;
    printf("NOGS: Lua start '%s'\n", path);
    DEBUG("NOGS: Lua start '%s'\n", path);
    f_chdrive(path);
    cn.throwError = 0;
    DEBUG("Memory_AllocLuaVM\n");
    cn.L = Memory_AllocLuaVM();
    DEBUG("luaL_openlibs\n");
    luaL_openlibs(cn.L);
    DEBUG("Register libs: Sys, HAL, App\n");
    LuaLibLevelSys_Register(cn.L);
    LuaLibLevelHAL_Register(cn.L);
    LuaLibLevelApp_Register(cn.L);
    lua_sethook(cn.L, luaHook, LUA_MASKLINE | LUA_MASKCOUNT, LINE_COUNT);
    int result = luaL_loadfile(cn.L, path);
    if (result) {
      char s[32 + 60] = "Can't open start file: ";
      switch (result) {
        case LUA_ERRSYNTAX : strncat(s, lua_tostring(cn.L, -1), 60); break;
        case LUA_ERRFILE   : strcat(s, "File not fount"); break;
        case LUA_ERRMEM    : strcat(s, "Out of memory"); break;
        default            : strcat(s, "Unknown");
      }
      DEBUG(s);
      SysLog_Send(SYSLOG_LUAERROR | SYSLOG_ERR, s);
      break;
    }
    result = lua_pcall(cn.L, 0, LUA_MULTRET, 0);
    if (result) {
      const char *error = lua_tostring(cn.L, -1);
      int errorLength = strlen(error);
      bool stop = strcmp(&error[errorLength - 4], "stop") == 0;
      restart = strcmp(&error[errorLength - 7], "restart") == 0;
      if (!stop && !restart) {
        DEBUG("Error: %s\n", error);
        SysLog_Send(SYSLOG_LUAERROR | SYSLOG_ERR, error);
      }
    }
    Memory_FreeLuaVM();
    cn.L = 0;
    printf("\nNOGS: Lua finished\n");
    DEBUG("\nNOGS: Lua finished\n");
  } while (restart);
}
/*---------------------------------------------------------------------------*/
