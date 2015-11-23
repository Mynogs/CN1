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
#ifndef HALLIS__H
#define HALLIS__H
/*===========================================================================
  HAL_
  ===========================================================================*/
#include "../Common.h" /*lint -e537 */
/*---------------------------------------------------------------------------*/
typedef struct XYZ16 {
  int16_t x, y, z;
} XYZ16;
/*---------------------------------------------------------------------------*/
typedef struct XYZ32 {
  int32_t x, y, z;
} XYZ32;
/*---------------------------------------------------------------------------*/
/*!
 * \brief LIS timer callback function
 * \param self The timer structure
 * \note The function is called with a rate of 1000Hz. The LIS samples with
 * a rate of 640Hz. So every sample is catched.
 */
void HALLIS_Server1MS(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int16_t HALLIS_GetXYZ(XYZ16 *xyz);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int32_t HALLIS_GetNoise(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int16_t HALLIS_Open(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void HALLIS_Close(void);
/*---------------------------------------------------------------------------*/
#endif
