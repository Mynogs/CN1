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
#ifndef SYSFFS_H
#define SYSFFS_H
/*===========================================================================
  FFS Far File System
  ===========================================================================*/
#include "Common.h"

#define FFS_SERVER_ERROR_NOT_ALLOWED -101
#define FFS_SERVER_ERROR_NO_PROCECT  -102
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int16_t FFS_Init(int8_t socketNumber, uint32_t ip);
/*---------------------------------------------------------------------------*/
bool FFS_GetState();
/*---------------------------------------------------------------------------*/
int16_t FFS_ReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead);
/*---------------------------------------------------------------------------*/
int16_t FFS_WriteBlocks(const void *buffer, uint32_t lba, uint32_t blocksToWrite);
/*---------------------------------------------------------------------------*/
#endif
