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
#ifndef LUALIBW5500_H
#define LUALIBW5500_H
/*===========================================================================
  Lua lib net
  ===========================================================================*/
/*---------------------------------------------------------------------------*/
#include "../Common.h" /*lint -e537 */
#include "../lua-5.3.0/src/lua.h"
/*---------------------------------------------------------------------------*/
/*!
 * \brief Serve the network.
 */
//void LuaLibW5500_Serve(lua_State *L);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void LuaLibW5500_Register(lua_State *L);
/*---------------------------------------------------------------------------*/
void LuaLibW5500_Init(void);
/*---------------------------------------------------------------------------*/
/*
 * \brief Get a free (unused) socket number. Start with the higest socket
 * number and count down. Socket 0 (for RAW mode) is the last choise.
 * \return >= 0: The socket number,  -1: All sockets in use
 */
int16_t LuaLibW5500_GetFreeSocket(void);
/*---------------------------------------------------------------------------*/
/*
 * \brief Get the socket 0 (for RAW mode)
 * \return == 0: Success,  -1: Socket 0 in use
 */
int16_t LuaLibW5500_GetSocket0(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Free a socket by number
 */
void LuaLibW5500_FreeSocket(uint8_t socketNumber);
/*---------------------------------------------------------------------------*/
uint16_t ChangeEndian16(uint16_t V);
/*-----------------------------------------------------------------------------*/
uint32_t ChangeEndian32(uint32_t V);
/*-----------------------------------------------------------------------------*/
/*!
 * \brief Convert string to IP number
 * \param s The string
 * \param ip Ref to IP number result in the form ll.lh.hl.hh
 * \return true if IP vas valid
 */
/*---------------------------------------------------------------------------*/
bool StringToIP(const char *s, uint32_t *ip);
/*!
 * \brief Convert IP number to string
 * \param s The result string
 * \param ip The IP number in the form ll.lh.hl.hh
 */
void IPToString(char *s, size_t size, uint32_t ip);
/*---------------------------------------------------------------------------*/
void MACToSTring(char *s, size_t size, uint8_t *mac);
/*---------------------------------------------------------------------------*/
#endif
