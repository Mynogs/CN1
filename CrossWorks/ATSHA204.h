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
#ifndef ATSHA204_H
#define ATSHA204_H
/*===========================================================================
  ATSHA204
  ===========================================================================*/
/*---------------------------------------------------------------------------*/
#include "Common.h" /*lint -e537 */
/*---------------------------------------------------------------------------*/
bool ATSHA204_Init(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief This function reads the serial number from the device
 * \param data9 An array of nine bytes
 * \return
 */
bool ATSHA204_GetSerialNumber(uint8_t *data9);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Generate a 32 byte random number
 * \param data Pointer to uint8 data[32]
 * \return true if random number has get
 */
bool ATSHA204_Random(uint8_t *data);
/*---------------------------------------------------------------------------*/
bool ATSHA204_ReadData(uint8_t *data);
/*---------------------------------------------------------------------------*/
#endif
