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
#ifndef LUALIBUTILS_H
#define LUALIBUTILS_H
/*===========================================================================
  Lua lib utils
  ===========================================================================*/
/*
  Usage

  V value
  S string
  B bool
  I integer
  F function
  T thread
  N name
  C channel
  P pin
  H handle
  L list
  J JSON string
  E nil,error
  . library.function
  : library:function
  @ self
  / Seperator for alternate list

*/
/*---------------------------------------------------------------------------*/
#include "../Common.h" /*lint -e537 */
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lauxlib.h"
/*---------------------------------------------------------------------------*/
typedef struct RegisterItem {
  char type;         // 'I'nteger, 'N'umber, 'F'unction
  const char *name;
  union {
    int i;
    float n;
    int (*f)(lua_State *L);
  } value;
  const char *usage;
} RegisterItem;
/*---------------------------------------------------------------------------*/
#define LIB_BEGIN(VAR, NAME) static const RegisterItem VAR [] = {{'L', NAME, {.i = 0}, 0},
#define LIB_END {0}};
#define LIB_ITEM_FUNCTION(NAME, FUNCTION) {'F', NAME, {.f = FUNCTION}, FUNCTION##Usage},
#define LIB_ITEM_INTEGER(NAME, INTEGER) {'I', NAME, {.i = INTEGER}, 0},
#define LIB_ITEM_NUMBER(NAME, NUMBER) {'N', NAME, {.n = NUMBER}, 0},
#define LIB_FUNCTION(NAME, USAGE) \
  static const char NAME##Usage[] = USAGE; \
  static int NAME(lua_State *L)
/*---------------------------------------------------------------------------*/
void LuaUtils_DumpTable(lua_State *L);
/*---------------------------------------------------------------------------*/
void LuaUtils_DumpStack(lua_State *L);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void LuaUtils_SetStopOnError(bool _stopOnError);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void LuaUtils_CheckParameter1(lua_State *L, uint16_t count, const char* usage);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int LuaUtils_CheckParameter2(lua_State *L, uint8_t countMin, uint16_t countMax, const char* usage);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Helper to return an error.
 * Function returns two things: nil,  text
 * \param L The Lua VM
 * \param format
 * \param ...
 */
int8_t LuaUtils_Error(lua_State *L, const char *format, ...);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void LuaUtils_Register(lua_State *L, const char *parentLibraryName, const char *libraryName, const RegisterItem *items);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
//extern const char LuaUtils_IntroduceUsage[];
//int LuaUtils_Introduce(lua_State *L, const RegisterItem *items);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 * \param L The Lua VM
 */
void* LuaUtils_ClassNew(lua_State *L, char *MetaName);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 * \param L The Lua VM
 */
void* LuaUtils_ClassGet(lua_State *L, int index, char *metaName);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 * \param L The Lua VM
 */
void* LuaUtils_ClassDelete(lua_State *L, char *metaName);
/*---------------------------------------------------------------------------*/
#endif
