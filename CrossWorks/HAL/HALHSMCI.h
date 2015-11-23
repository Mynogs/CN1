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
#ifndef HALHSMCI_H
#define HALHSMCI_H
/*---------------------------------------------------------------------------*/
#include "../Common.h" /*lint -e537 */
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int16_t HALHSMCI_Init(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void HALHSMCI_Deinit(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Read from card
 */
//
int16_t HALHSMCI_ReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Write to card
 */
int16_t HALHSMCI_WriteBlocks(const void *buffer, uint32_t lba, uint32_t blocksToWrite);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get card size
 */
void HALHSMCI_GetSizeInfo(uint32_t *availableBlocks, uint16_t *sizeOfBlock);
/*---------------------------------------------------------------------------*/
#endif /* SD_HSMCI_H_ */
