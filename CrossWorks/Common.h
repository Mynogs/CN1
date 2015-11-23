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
#ifndef COMMON_H
#define COMMON_H
#include "Nogs.h"
/*---------------------------------------------------------------------------*/
/*!
 * \brief Configuration section
 */
//#define LUA52
#define LUA53
/*---------------------------------------------------------------------------*/
#define TFTP_BOOT_FLASH 
#define TFTP_BOOT_SD
#define USEFFS
//#define REDIRECT_STDOUT_TO_RS232
//#define REDIRECT_STDOUT_TO_DEBUG 
#define REDIRECT_STDOUT_TO_SYSLOG
/*---------------------------------------------------------------------------*/
/*!
 * \brief Select board type
 */
#define BOARD_STAMP
//#define IS_NUCLEO
/*---------------------------------------------------------------------------*/
#define NOGS_TYPE CN
#ifdef BOARD_STAMP
  #define NOGS_DEVICE Stamp160k_V2.0g
  #define HAS_ATSHA204
  #define HAS_WS2812
  #define HAS_I2C
  #define HAS_EVE
  //#define HAS_LIS
  //#define HAS_GYR_DISPLAY
  #define HAS_SOFT_PWM
  #define HAS_ADC
  #define HAS_SOFT_COUNTER
  #define HAS_WATCHDOG
#endif
/*---------------------------------------------------------------------------*/
#define NET_ZEROCONF
/*---------------------------------------------------------------------------*/
#define LED_WAIT_FOR_LINK  0xF0000000
#define LED_ZEROCONF       0xF0F00000
#define LED_RUNNING        0x00000000
/*---------------------------------------------------------------------------*/
/*!
 * \brief Datatypes
 */
typedef unsigned char          uchar;
/*---------------------------------------------------------------------------*/
/*!
 * \brief bool definition
 * \note This is the platform independen form
 */
#define bool uint8_t
#define true (0 == 0)
#define false (0 != 0)
/*---------------------------------------------------------------------------*/
/*!
 * \brief Macros
 */
#define PACKED __attribute__ ((packed))
#define WEAK __attribute__ ((weak))
#define BYTE(B7,B6,B5,B4,B3,B2,B1,B0) (((B7) << 7)|((B6) << 6)|((B5) << 5)|((B4) << 4)|((B3) << 3)|((B2) << 2)|((B1) << 1)|(B0))
#define BIT(B) (1<<(B))
#define CONCAT2I(A,B) A##B
#define CONCAT2(A,B) CONCAT2I(A,B)
#define CONCAT3I(A,B,C) A##B##C
#define CONCAT3(A,B,C) CONCAT3I(A,B,C)
#define CONCAT4I(A,B,C,E) A##B##C##E
#define CONCAT4(A,B,C,E) CONCAT4I(A,B,C,E)
/*---------------------------------------------------------------------------*/
/*!
 * \brief Include
 */
//#if !lint
  #include "SAM_Series.h"
  #include "sam4s.h"
//#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*!
 * \brief Some checks
 * \note See definition in "SAM4S_Configure.s"
 */
#ifndef _SAM4S_SERIES
  #error _SAM4S_SERIES
#endif
#if (__TARGET_PROCESSOR != SAM4S16B)
  #error __TARGET_PROCESSOR must be SAM4S16B
#endif
#if CHIP_FREQ_XTAL != 16000000UL
  #error CHIP_FREQ_XTAL != 16000000UL
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  #ifndef NO_WATCHDOG_DISABLE
    #error Watchdog is DISABLED by project settings!
  #endif
#else
  #ifdef NO_WATCHDOG_DISABLE
    #error Watchdog is ENABLED by project settings! You have 16 seconds...
  #endif
#endif
/*---------------------------------------------------------------------------*/
#if !lint
  #define sprintf !!Dont use it!!
#endif
/*---------------------------------------------------------------------------*/
#endif
