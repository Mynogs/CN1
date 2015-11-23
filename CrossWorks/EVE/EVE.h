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
#ifndef EVE__H
#define EVE__H
/*===========================================================================
  HAL
  ===========================================================================*/
#include "../Common.h" /*lint -e537 */
//#include "FT800.h"
/*---------------------------------------------------------------------------*/
#define RESULT_EVE_ERROR_COPROCESSOR -50
/*---------------------------------------------------------------------------*/
typedef struct LCD {
  uint16_t width;  // Active width of LCD display
  uint16_t height; // Active height of LCD display
  uint16_t hCycle; // Total number of clocks per line
  uint16_t hOffset;// Start of active line
  uint16_t hSync0; // Start of horizontal sync pulse
  uint16_t hSync1; // End of horizontal sync pulse
  uint16_t vCycle; // Total number of lines per screen
  uint16_t vOffset;// Start of active screen
  uint16_t vSync0; // Start of vertical sync pulse
  uint16_t vSync1; // End of vertical sync pulse
  uint8_t pClk;    // Pixel Clock
  uint8_t swizzle; // Define RGB output pins
  uint8_t pclkPol; // Define active edge of PCLK
} LCD;
/*---------------------------------------------------------------------------*/
extern const LCD _LCD_QVGA;
extern const LCD _LCD_WQVGA;
/*---------------------------------------------------------------------------*/
/*!
 * \brief Open the EVE SPI interface
 */
int16_t EVE_Open(uint16_t channel);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Close the EVE SPI interface
 */
void EVE_Close(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Begin to append items to the graphic engine.
 * \note Wait for graphic engine command buffer is empty
 */
int16_t EVE_CommandBegin(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Aappend item to the graphic engine command buffer.
 */
int16_t EVE_CommandAppend(uint32_t item);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Aappend item to the graphic engine command buffer.
 */
int16_t EVE_CommandAppendString(const char *s);
/*---------------------------------------------------------------------------*/
/*!
 * \brief End append items to the graphic engine.
 * Add DL_DISPLAY and DL_SWAP to make it visible
 */
int16_t EVE_CommandEnd(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Copy to main graphics RAM
 */
int16_t EVE_WriteMain(uint32_t address, uint8_t *data, uint16_t size);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Copy to main palette RAM
 */
int16_t EVE_WritePalette(uint32_t address, uint8_t *data, uint16_t size);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void EVE_DisplayListTest(void);
/*---------------------------------------------------------------------------*/

#endif


















