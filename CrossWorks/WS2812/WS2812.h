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
#ifndef WS2812H
#define WS2812H
/*---------------------------------------------------------------------------*/
/*!
 * \file WS2812.h
 * \brief Driver for the Adafruid NeoPixle (WS2812) LED
 *
 * \verbatim
 *
 * $Log$
 * Revision 1.0a  2014/03/09
 * AR First version
 * Revision 1.0b  2014/03/16
 * AR Change rgb to gbr
 * Revision 1.0c  2014/11/08
 * AR Port to ARM4S
 * AR Change back to rgb. LED wrong order is no compensated
 * \endverbatim
 */
/*-----------------------------------------------------------------------------*/
#include "../Common.h" /*lint -e537 */
/*-----------------------------------------------------------------------------*/
/*!
 * \brief Send 24bit rgb data to the chain of LEDs using the WS2811/12 chip.
 * These are Adafruits NeoPixel products. Chis is from Worldsemi.
 * \param rgb List of count RGB patterns. Size must be multiple of three
 * \param count Number of elements in the list. Must be multiple of three
 * \param id Pin id
 * \note One RGB pattern (count == 3) takes 28µs. 16 LEDs takes 462µs, so the
 * maximum update rate for 16 LEDs is 2200Hz. For this time all interrups are
 * disabled!
 * \note To load the data into LEDs pwm unit a gab of min 50µ must be between
 * two calls of WS2812_Send.
 */
void WS2812_Send(uint8_t *rgb, uint16_t count, bool negate, int16_t id);
/*-----------------------------------------------------------------------------*/
#endif
