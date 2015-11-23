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
#ifndef CN_H
#define CN_H
/*---------------------------------------------------------------------------*/
//#include "lua-5.2.3/src/lua.h"
#include "../../lua-5.3.0/src/lua.h"
/*---------------------------------------------------------------------------*/
#define CN_VERSION    "CN 2.0.1"
#define CN_COPYRIGHT  "Copyright (C) Andre Riesberg"
#define CN_AUTHORS    "Andre Riesberg"
/*---------------------------------------------------------------------------*/
/*!
 * \brief Datatypes
 */
typedef unsigned char uchar;
/*---------------------------------------------------------------------------*/
/*!
 * \brief bool definition
 * \note This is the platform independen form
 */
#define bool uint8_t
#define true (0 == 0)
#define false (0 != 0)
/*---------------------------------------------------------------------------*/
/*!
 * \brief The three base librarys for NOGS
 */
/*
extern void __attribute__ ((weak)) LuaLibLevelSys_Register(lua_State *L) {}
extern void __attribute__ ((weak)) LuaLibLevelHAL_Register(lua_State *L) {}
extern void __attribute__ ((weak)) LuaLibLevelApp_Register(lua_State *L) {}
*/
/*---------------------------------------------------------------------------*/
void CN_Init(void);
/*---------------------------------------------------------------------------*/
//void CN_ServeTick(void);
/*---------------------------------------------------------------------------*/
void CN_TraceDump(void);
/*---------------------------------------------------------------------------*/
void CN_Stop(void);
/*---------------------------------------------------------------------------*/
void CN_Restart(void);
/*---------------------------------------------------------------------------*/
void CN_Run(const char *path);
/*---------------------------------------------------------------------------*/
#endif
