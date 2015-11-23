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
#ifndef LUALIBMEMORY_H
#define LUALIBMEMORY_H
/*===========================================================================
  Memory
  ===========================================================================*/
#include "Common.h" /*lint -e537 */
#include "lua-5.3.0/src/lua.h"
/*---------------------------------------------------------------------------*/
#define M_SAVE      (1 << 0)
#define M_CLEAR     (1 << 1)
/*---------------------------------------------------------------------------*/
/*!
 * \brief Init memory
 */
void Memory_Init(void);
/*---------------------------------------------------------------------------*/
void Memory_StartTracker(int8_t socketNumber, uint32_t ip);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Task save memory allocation
 */
void* __Memory_Allocate(uint32_t size, uint8_t flags, char *file, int line);
#define Memory_Allocate(SIZE, FLAGS) __Memory_Allocate(SIZE, FLAGS, __FILE__, __LINE__)
/*---------------------------------------------------------------------------*/
/*!
 * \brief Allocate a copy of a string. The copy has the length off
 * strlen(s) + 1
 */
uchar* Memory_NewString(uchar *s, uint8_t flags);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Allocate a copy of a string. The copy has the length off
 * maLength + 1
 */
uchar* Memory_NewStringLimited(uchar *s, uint32_t maxLength, uint8_t flags);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Allocate a Lua VM. Lua's memory functions are set to be compatile
 * to this memory pool.
 */
lua_State* Memory_AllocLuaVM(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Free the memory. ptr can be 0
 */
void __Memory_Free(void* ptr, uint32_t size, char *file, int line);
#define Memory_Free(PTR, SIZE) __Memory_Free(PTR, SIZE, __FILE__, __LINE__)
/*---------------------------------------------------------------------------*/
/*!
 * \brief Free a lua VM. ptr can be 0
 */
void Memory_FreeLuaVM();
/*---------------------------------------------------------------------------*/
void Memory_Dump(void);
/*---------------------------------------------------------------------------*/

#endif
