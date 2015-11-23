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
#ifndef MODIFYER_H
#define MODIFYER_H
/*===========================================================================
  HAL
  ===========================================================================*/
#include "Common.h" /*lint -e537 */
/*---------------------------------------------------------------------------*/
/*!
 * \brief Modify a 32bit value at a given address
 * \param address Address to modify
 * \param mask Mask out bits. Also mask value.
 * \param value Value
 */
void Modifyer_32(volatile uint32_t *address, uint32_t mask, uint32_t value);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Modify a 32bit value at a given address. Value is automatic
 * shifted left to match the mask
 * \param address Address to modify
 * \param mask Mask out bits. Also mask value.
 * \param value Value
 */
void Modifyer_32AutoShift(volatile uint32_t *address, uint32_t mask, uint32_t value);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Modify a 32bit value at a given address by name
 * \param name sName table,  last element must be a null pointer
 * \param address Address to modify
 * \param mask Mask out bits. Also mask value.
 * \param value Base value,  incremented by lowest set bit im mask per name
 * \return == 0 success,  < 0 error
 */
int8_t Modifyer_32ByName(const char *names[], volatile uint32_t *address, uint32_t mask, uint32_t value, const char *name);
/*---------------------------------------------------------------------------*/
#endif