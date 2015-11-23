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
#include "LuaLibUtils.h"
#include "../lua-5.3.0/src/lua.h"
#include "../lua-5.3.0/src/lauxlib.h"
#include "../lua-5.3.0/src/lobject.h"
#include "../lua-5.3.0/src/lstate.h"
#include "../lua-5.3.0/src/lopcodes.h"
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
bool stopOnError = true;
/*---------------------------------------------------------------------------*/
static void ExpandUsageToBuffer(luaL_Buffer *buffer, const char *usage, const char *library, const char *function)
{
  luaL_addstring(buffer, "[\"");
  while (*usage) {
    switch (*usage) {
      case 'V' : luaL_addstring(buffer, "value"); break;
      case 'S' : luaL_addstring(buffer, "string"); break;
      case 'B' : luaL_addstring(buffer, "bool"); break;
      case 'I' : luaL_addstring(buffer, "integer"); break;
      case 'F' : luaL_addstring(buffer, "function"); break;
      case 'T' : luaL_addstring(buffer, "thread"); break;
      case 'N' : luaL_addstring(buffer, "name"); break;
      case 'C' : luaL_addstring(buffer, "channel"); break;
      case 'P' : luaL_addstring(buffer, "pin"); break;
      case 'H' : luaL_addstring(buffer, "handle"); break;
      case 'L' : luaL_addstring(buffer, "list"); break;
      case 'J' : luaL_addstring(buffer, "json-string"); break;
      case 'E' : luaL_addstring(buffer, "nil, error"); break;
      case '@' : luaL_addstring(buffer, "self"); break;
      case '!' : luaL_addstring(buffer, "config"); break;
      case '/' : luaL_addstring(buffer, ")\",\""); break;

      case '.' :
        if (buffer->n > 2) luaL_addchar(buffer, '=');
        luaL_addstring(buffer, library);
        luaL_addchar(buffer, '.');
        luaL_addstring(buffer, function);
        luaL_addchar(buffer, '(');
        break;

      case ':' :
        if (buffer->n > 2) luaL_addchar(buffer, '=');
        luaL_addstring(buffer, library);
        luaL_addchar(buffer, ':');
        luaL_addstring(buffer, function);
        luaL_addchar(buffer, '(');
        break;

      default : luaL_addchar(buffer, *usage);
    }
    usage++;
  }
  luaL_addstring(buffer, ")\"]");
}
/*---------------------------------------------------------------------------*/
void LuaUtils_SetStopOnError(bool _stopOnError)
{
  stopOnError = _stopOnError;
}
/*---------------------------------------------------------------------------*/
void LuaUtils_DumpTable(lua_State *L)
{
  uint8_t i;
  lua_pushnil(L);
  for (i = 0; i <= 9;i++) {
    if (!lua_next(L, -2)) 
      return;
    lua_pushvalue(L, -1); // Push value
    lua_pushvalue(L, -3); // Stack: -1: key, -2: value
    {
      int type = lua_type(L, -1);
      switch (type) {
        case LUA_TBOOLEAN:
          DEBUG(" [%s]", lua_toboolean(L, -1) ? "true" : "false");
          break;
        case LUA_TNUMBER:
          DEBUG(" [%s]", lua_tostring(L, -1));
          break;
        default:
          DEBUG(" ['%s']", lua_tostring(L, -1));
      }
    }
      {
        int type = lua_type(L, -2);
        switch (type) {
        case LUA_TSTRING:
          DEBUG("='%s'", lua_tostring(L, -2));
          break;
        case LUA_TBOOLEAN:
          DEBUG("=%s", lua_toboolean(L, -2) ? "true" : "false");
          break;
        case LUA_TNUMBER:
          DEBUG("=%s", lua_tostring(L, -2));
          break;
        default :
          DEBUG("=<%s>", lua_typename(L, lua_type(L, -2)));
      }
    }
    lua_pop(L, 3);
  }
  lua_pop(L, 1);
  DEBUG(" ...");
}
/*---------------------------------------------------------------------------*/
void LuaUtils_DumpStack(lua_State *L)
{
  int top = lua_gettop(L);
  DEBUG("\nStackdump\n");
  for (int8_t i = -1; i >= -10; i--) {
    int8_t a = top + i + 1;
    if (a >= 1)
      DEBUG("%3d", a);
    else
      DEBUG("   ");
    DEBUG("%4d  ", i);
    switch (lua_type(L, i)) {

      case LUA_TSTRING:
        DEBUG("string '%s'\n", lua_tostring(L, i));
        break;

      case LUA_TBOOLEAN:
        DEBUG("%s\n", lua_toboolean(L, i) ? "true" : "false");
        break;

      case LUA_TNUMBER:
        DEBUG("number %s\n", lua_tostring(L, i));
        break;

      case LUA_TTABLE:
        DEBUG("table: ");
        lua_pushvalue(L, i);
        //DumpTable(L);
        lua_pop(L, 1);
        DEBUG("\n");
        break;

      default:
        DEBUG("%s\n", lua_typename(L, lua_type(L, i)));
        break;

    }
  }
}
/*---------------------------------------------------------------------------*/
void LuaUtils_CheckParameter1(lua_State *L, uint16_t count, const char* usage)
{
  int n = lua_gettop(L);
  if (n != count) 
    luaL_error(L, "Got %d arguments expected %d (%s)", n, count, usage);
}
/*---------------------------------------------------------------------------*/
int LuaUtils_CheckParameter2(lua_State *L, uint8_t countMin, uint16_t countMax, const char* usage)
{
  int n = lua_gettop(L);
  if ((n < countMin) || (n > countMax)) 
    luaL_error(L, "Got %d arguments expected %d to %d (%s)", n, countMin, countMax, usage);
  return n;
}
/*---------------------------------------------------------------------------*/
int8_t LuaUtils_Error(lua_State *L, const char *format, ...)
{
  va_list args;
  va_start(args,  format);
  lua_pushnil(L);
  lua_pushvfstring(L, format, args);
  if (stopOnError)
    luaL_error(L, "[sys.stopOnError == true] %s", luaL_checkstring(L, -1));
  va_end(args);
  return 2;
}
/*---------------------------------------------------------------------------*/
static const char* LuaUtils_IntroduceUsage = "J:[FN]/J.@[,FN]";
int LuaUtils_Introduce(lua_State *L)
{
  int n = LuaUtils_CheckParameter2(L, 1, 2, LuaUtils_IntroduceUsage);
  if (!lua_istable(L, 1))
    return LuaUtils_Error(L, "Not a table");

  if (n >= 2) {

    bool found = false;
    lua_pushstring(L, "__intrinsic");
    lua_gettable(L, -3);

    if (lua_islightuserdata(L, -1)) {

      const RegisterItem *items = lua_touserdata(L, -1);
      const char *library = items[0].name;
      const char *function = lua_tostring(L, 2);
      const char *usage = 0;

      for (const RegisterItem *item = items + 1; item->type; item++) {
        if (strcmp(function, "introduce") == 0)
          usage = LuaUtils_IntroduceUsage;
        if (strcmp(function, item->name) == 0)
          usage = item->usage;
        if (usage)
          break;
      }
      if (usage) {
        luaL_Buffer buffer;
        luaL_buffinit(L, &buffer);
        ExpandUsageToBuffer(&buffer, usage, library, function);
        luaL_pushresult(&buffer);
      } else {
        lua_pushstring(L, function);
        lua_gettable(L, -4);
        if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1)) {
          Proto *proto = ((LClosure*) lua_topointer(L, -1))->p;
          luaL_Buffer buffer;
          luaL_buffinit(L, &buffer);
          luaL_addstring(&buffer, "[\"");
          luaL_addstring(&buffer, library);
          luaL_addchar(&buffer, '.');
          luaL_addstring(&buffer, function);
          luaL_addchar(&buffer, '(');
          for (uint16_t i = 0; i < proto->numparams; i++) {
            if (i) luaL_addchar(&buffer, ',');
            luaL_addstring(&buffer, getstr(proto->locvars[i].varname));
          }
          if (proto->is_vararg) {
            if (proto->numparams) luaL_addchar(&buffer, ',');
            luaL_addstring(&buffer, "...");
          }
          luaL_addstring(&buffer, ")\"]");
          luaL_pushresult(&buffer);
        } else
          lua_pushstring(L, "<unknown>");
      }
    }

  } else {

    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);
    luaL_addchar(&buffer, '[');

    uint16_t i = 0;
    {
      lua_pushnil(L);
      while (lua_next(L, -2)) {

        switch (lua_type(L, -1)) {

          case LUA_TBOOLEAN :
            // {"name":boolean},
            if (i++)
              luaL_addchar(&buffer, ',');
            luaL_addstring(&buffer, "{\"");
            luaL_addstring(&buffer, lua_tostring(L, -2));
            luaL_addstring(&buffer, "\":");
            luaL_addstring(&buffer, lua_toboolean(L, -1) ? "true" : "false");
            luaL_addchar(&buffer, '}');
            break;

          case LUA_TNUMBER :
            // {"name":number},
            if (i++)
              luaL_addchar(&buffer, ',');
            luaL_addstring(&buffer, "{\"");
            luaL_addstring(&buffer, lua_tostring(L, -2));
            luaL_addstring(&buffer, "\":");
            luaL_addstring(&buffer, lua_tostring(L, -1));
            luaL_addchar(&buffer, '}');
            break;

          case LUA_TUSERDATA :
          case LUA_TLIGHTUSERDATA :
            // "name",
            if (i++)
              luaL_addchar(&buffer, ',');
            luaL_addchar(&buffer, '"');
            luaL_addstring(&buffer, lua_tostring(L, -2));
            luaL_addchar(&buffer, '"');
            break;

        }
        lua_pop(L, 1);
      }
    }
    {
      lua_pushnil(L);
      while (lua_next(L, -2)) {
        if (lua_type(L, -1) == LUA_TFUNCTION) {
          // "name()",
          if (i++)
            luaL_addchar(&buffer, ',');
          luaL_addchar(&buffer, '"');
          luaL_addstring(&buffer, lua_tostring(L, -2));
          luaL_addstring(&buffer, "()\""); // TODO Hier "Usage"
        }
        lua_pop(L, 1);
      }
    }
    {
      lua_pushnil(L);
      while (lua_next(L, -2)) {
        if (lua_type(L, -1) == LUA_TTABLE) {
          // "name",
          if (i++)
            luaL_addchar(&buffer, ',');
          luaL_addchar(&buffer, '"');
          luaL_addstring(&buffer, lua_tostring(L, -2));
          luaL_addstring(&buffer, ".*\"");
        }
        lua_pop(L, 1);
      }
    }
    luaL_addchar(&buffer, ']');
    luaL_pushresult(&buffer);

  }
  return 1;
}
/*---------------------------------------------------------------------------*/
void LuaUtils_Register(lua_State *L, const char *parentLibraryName, const char *libraryName, const RegisterItem *items)
{
  if (parentLibraryName) {
    lua_getglobal(L, parentLibraryName);
    if (!lua_istable(L, -1)) return;
  }
  // Count the items in the table
  int count = 0;
  for (const RegisterItem *item = items; item->type; item++) count++;
  // Create the table
  if (parentLibraryName)
    lua_pushstring(L, libraryName);
  lua_createtable(L, 0, count);

  lua_pushstring(L, "__intrinsic");
  lua_pushlightuserdata(L, (void*) items);
  lua_settable(L, -3);

  lua_pushstring(L, "introduce");
  lua_pushcfunction(L, LuaUtils_Introduce);
  lua_settable(L, -3);

  for (const RegisterItem *item = items + 1; item->type; item++) {
    lua_pushstring(L, item->name);
    switch (item->type) {

      //case LUA_TBOOLEAN :
      //case LUA_TLIGHTUSERDATA :

      case 'I' :
        lua_pushinteger(L, item->value.i);
        break;

      case 'N' :
        lua_pushnumber(L, item->value.n);
        break;

      //case LUA_TSTRING    4
      //case LUA_TTABLE    5

      case 'F' :
        lua_pushcfunction(L, item->value.f);
        break;

    }
    lua_settable(L, -3);
  }
  if (parentLibraryName)
    lua_settable(L, -3);
  else
    lua_setglobal(L, libraryName);
}

/*
  int count = 0;
  for (const luaL_Reg *lib = Lib; lib->name; lib++) count++;
  lua_createtable(L, 0, count);
  for (const luaL_Reg *lib = Lib; lib->name; lib++) {
    lua_pushstring(L, lib->name);
    lua_pushcfunction(L, lib->func);
    lua_settable(L, -3);
  }
  lua_setglobal(L, "sys");
*/

/*---------------------------------------------------------------------------*/
// Lua class support
/*---------------------------------------------------------------------------*/
void* LuaUtils_ClassNew(lua_State *L, char *MetaName)
{
  void *UserData;
  DEBUG("LuaClassNew %s\n", MetaName);
  luaL_checktype(L, 1, LUA_TTABLE);             // First argument is now a table that represent the class to instantiate
  lua_newtable(L);                              // Create table to represent instance and push it on the stack
  lua_pushvalue(L, 1);                          // Set first argument of new to metatable of instance
  lua_setmetatable(L, -2);                      // Pops the table and sets it as the new metatable for instance
  lua_pushvalue(L, 1);                          // Do function lookups in metatable
  lua_setfield(L, 1, "__index");                // so set __index = 1
  UserData = lua_newuserdata(L, sizeof(void*)); // Allocate memory for a pointer to to object
  DEBUG("LuaClassNew UserData = %08X\n", UserData);
  luaL_getmetatable(L, MetaName);               // Get metatable store in the registry
  lua_setmetatable(L, -2);                      // Set user data for TUDP to use this metatable
  lua_setfield(L, -2, "__self");                // Set field '__self' of instance table to the user data
  DEBUG("LuaClassNew\n");
  return UserData;
}
/*---------------------------------------------------------------------------*/
void* LuaUtils_ClassGet(lua_State *L, int index, char *metaName)
{
  void *userData;
  DEBUG("LuaClassGet %d %s\n", index, metaName);
  luaL_checktype(L, index, LUA_TTABLE);          // Must be a table
  lua_getfield(L, index, "__self");              // Get self and push it on the stack
  userData = luaL_checkudata(L, -1, metaName);
  DEBUG("LuaClassGet userData = %08X\n", userData);
  luaL_argcheck(L, userData, 1, "Class expected");
  return userData;
}
/*---------------------------------------------------------------------------*/
void* LuaUtils_ClassDelete(lua_State *L, char *metaName)
{
  void *userData;
  DEBUG("LuaClassDelete %s\n", metaName);
  userData = luaL_checkudata(L, 1, metaName);
  DEBUG("LuaClassDelete userData = %08X\n", userData);
  luaL_argcheck(L, userData, 1, "Class expected");
  return userData;
}
/*---------------------------------------------------------------------------*/








