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
#ifndef ZEROCONF_H
#define ZEROCONF_H
/*===========================================================================
  Zeroconf
  ===========================================================================*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Implementation of zeroconf.
 */
/*---------------------------------------------------------------------------*/
//#define ZEROCONFIG_FAST
/*---------------------------------------------------------------------------*/
#include "../Common.h" /*lint -e537 */
#include "../HAL/HAL.h"
/*---------------------------------------------------------------------------*/
#define RESULT_ZEROCONFIG_ERROR_SOCKET0_IN_USE      -101
#define RESULT_ZEROCONFIG_ERROR_CANT_OPEN_SOCKET    -102
#define RESULT_ZEROCONFIG_ERROR_CANT_SEND_ARP_PROBE -103
#define RESULT_ZEROCONFIG_ERROR_GIVE_UP             -104
#define RESULT_ZEROCONFIG_IP_FREE                   RESULT_OK
/*---------------------------------------------------------------------------*/
int16_t ZeroConfig(uint8_t *thisMAC, uint32_t *freeIP);
/*---------------------------------------------------------------------------*/
#endif
