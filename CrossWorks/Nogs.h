/*---------------------------------------------------------------------------*/
/*
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
/*---------------------------------------------------------------------------*/
#ifndef NOGS_H
#define NOGS_H
/*===========================================================================

  ===========================================================================*/
/*---------------------------------------------------------------------------*/
#define NOGS_BANNER "\n _  _  _  _\n| || ||  |_\n| ||_||_| _|"
/*---------------------------------------------------------------------------*/
/*!
 * \brief Configuration section
 */
enum Nogs_NodeType {nntUnknown = 0, nntPN = 1, nntCN = 2, nntSN = 3, nntAN = 4};
/*---------------------------------------------------------------------------*/
#ifndef NOGS_TYPE
  #define NOGS_TYPE UNKNOWN_NODE
#endif
#ifndef NOGS_DEVICE
  #define NOGS_DEVICE UNKNOWN_DEVICE
#endif
/*---------------------------------------------------------------------------*/
#define __GNU_AUTOMAKE__ 0
/*---------------------------------------------------------------------------*/
#if defined(__BORLANDC__) && (__BORLANDC__ <= 0x0550)
  typedef signed char       int8_t;
  typedef unsigned char     uint8_t;
  typedef unsigned char     bool_t;
  typedef unsigned char     uchar_t;
  typedef signed short      int16_t;
  typedef unsigned short    uint16_t;
  typedef signed long int   int32_t;
  typedef unsigned long int uint32_t;
  #define prog_uchar_t uchar_t
  #define prog_char_t char
  #define true (0 == 0)
  #define false (0 != 0)
  #define pgm_read_byte(ADDRESS) (*(ADDRESS))
  #define PSTR(S) S
  #define strcmp_P strcmp
#elif __GNU_AUTOMAKE__ > 0
  //typedef short int int16_t;
  typedef short int int;
#else
  #include <stdint.h>
  #define true (0 == 0)
  #define false (0 != 0)
#endif
typedef uint8_t bool_t;
#ifdef __AVR_ARCH__
  #include <avr/pgmspace.h>
#endif
/*---------------------------------------------------------------------------*/
#define BIT(B) (1 << (B))
#define BIT_IS_SET(X, B) (((X) & BIT(B)) != 0)
#define BIT_IS_CLEAR(X, B) (((X) & BIT(B)) == 0)
#define BYTE(B7, B6, B5, B4, B3, B2, B1, B0) (((B7) << 7) | ((B6) << 6) | ((B5) << 5) | ((B4) << 4) | ((B3) << 3) | ((B2) << 2) | ((B1) << 1) | (B0))
/*---------------------------------------------------------------------------*/
#define STRINGIFY(X) #X
#define TOSTRING(X) STRINGIFY(X)
#define CONCAT(S1, S2) S1##S2
/*---------------------------------------------------------------------------*/
#define RESULT_OK     0
#define RESULT_ERROR -1
/*---------------------------------------------------------------------------*/
#endif