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
#ifndef HALW5500_H
#define HALW5500_H
/*===========================================================================
  HAL
  ===========================================================================*/
#include "../Common.h" /*lint -e537 */
/*---------------------------------------------------------------------------*/
#define IP_MAKE(A, B, C, D) \
  (((uint32_t) (D) << 24) + ((uint32_t) (C) << 16) + ((uint32_t) (B) << 8) + (uint32_t) (A))
#define IP_NULL         IP_MAKE(0, 0, 0, 0)
#define IP_BROADCAST    IP_MAKE(255, 255, 255, 255)
/*---------------------------------------------------------------------------*/
#define MASK_CLASS_A    IP_MAKE(255, 0, 0, 0)
#define MASK_CLASS_B    IP_MAKE(255, 255, 0, 0)
#define MASK_CLASS_C    IP_MAKE(255, 255, 255, 0)
/*---------------------------------------------------------------------------*/
bool HALW5500_Init(uint8_t *mac);
/*---------------------------------------------------------------------------*/
bool HALW5500_IsLinkUp(void);
/*---------------------------------------------------------------------------*/
bool HALW5500_WaitForLinkUp(int16_t timeoutS);
/*---------------------------------------------------------------------------*/
int8_t HALW5500_Config(const char *config, uint8_t configNumber);
/*---------------------------------------------------------------------------*/
int8_t HALW5500_SetIP(uint32_t ip, uint32_t mask);
/*---------------------------------------------------------------------------*/
int8_t HALW5500_SetGateway(uint32_t gateway);
/*---------------------------------------------------------------------------*/
#endif

