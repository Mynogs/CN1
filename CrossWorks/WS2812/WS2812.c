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
#include "WS2812.h"
#include "../HAL/HAL.h"
/*---------------------------------------------------------------------------*/
#undef DEBUG
#ifdef STARTUP_FROM_RESET
  #define DEBUG(...)
#else
  #if 1
    extern int debug_printf(const char *format, ...);
    #define DEBUG debug_printf
  #else
    #define DEBUG(...)
  #endif
#endif
/*---------------------------------------------------------------------------*/
#if (__TARGET_PROCESSOR != SAM4S16B)
  #error __TARGET_PROCESSOR must be SAM4S16B
#endif
/*---------------------------------------------------------------------------
  WS2812
  ---------------------------------------------------------------------------*/
/*!
 *  - = 25ns
 *                        ____+/-150ns                    ____+/-150ns
 *                       /    \                          /    \
 *  0         --------------
 *  ----------              --------------------------------
 *            \___350ns____/\____________800ns_____________/
 *                T0H                    T0L
 *
 *                                      ____+/-150ns            ____+/-150ns
 *                                     /    \                  /    \
 *  1         ----------------------------
 *  ----------                            ------------------------
 *            \__________700ns___________/\_________600ns________/
 *                       T1H                        T1L
 *
 *
 *    Composition of 24bit data:
 *    G7 G6 G5 G4 G3 G2 G1 G0 R7 R6 R5 R4 R3 R2 R1 R0 B7 B6 B5 B4 B3 B2 B1 B0
 */
/*---------------------------------------------------------------------------*/
#define T0H 350
#define T1H 700
#define T0L 800
#define T1L 600
/*---------------------------------------------------------------------------*/
#define NOP1() \
  __asm volatile("mov r0,r0")
/*---------------------------------------------------------------------------*/
#define NOP2() \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0") \
/*---------------------------------------------------------------------------*/
#define NOP4() \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0")
/*---------------------------------------------------------------------------*/
#define NOP8() \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0")
/*---------------------------------------------------------------------------*/
#define NOP16() \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0"); \
  __asm volatile("mov r0,r0")
/*---------------------------------------------------------------------------*/
// 700ns  600ns
// 800ns  450ns
#define H { \
    pio->PIO_SODR = mask; \ 
    NOP16(); \
    NOP16(); \
    NOP16(); \
    NOP8(); \
    NOP4(); \
    NOP4(); \
    NOP4(); \
    NOP4(); \
    NOP2(); \
  \
    pio->PIO_CODR = mask; \ 
    NOP16(); \
    NOP8(); \
    NOP4(); \
  }
/*---------------------------------------------------------------------------*/
// 350ns  800ns (WS2812)
// 400ns  850ns (WS2812B)
#define L { \
    pio->PIO_SODR = mask; \ 
    NOP16(); \
    NOP8(); \
    NOP4(); \
    NOP2(); \
    NOP2(); \
    NOP2(); \
  \
    pio->PIO_CODR = mask; \ 
    NOP16(); \
    NOP16(); \
    NOP16(); \
    NOP8(); \
    NOP4(); \
    NOP2(); \
    NOP2(); \
    NOP2(); \
  }
/*---------------------------------------------------------------------------*/
/*!
 * \brief Disable flash optimization
 * \return Previus FMR register value
 */
static uint32_t __attribute__ ((long_call, section (".fast"))) FlashOptimizationDisable(void)
{
  uint32_t FMR = EFC0->EEFC_FMR;                     // Remember old register value
  EFC0->EEFC_FMR |= EEFC_FMR_SCOD;                   // Sequential code optimization is disabled
  EFC0->EEFC_FMR &= ~(EEFC_FMR_FAM | EEFC_FMR_CLOE); // 64 bit access (slow), opcode loops optimization is disabled
  while ((EFC0->EEFC_FSR & EEFC_FSR_FRDY) == 0) {}
  return FMR;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Restore flash optimization
 * \param FMR New register value
 */
static void __attribute__ ((long_call, section (".fast"))) FlashOptimizationRestore(uint32_t FMR)
{
  EFC0->EEFC_FMR = FMR;
  while ((EFC0->EEFC_FSR & EEFC_FSR_FRDY) == 0) {}
}
/*---------------------------------------------------------------------------*/
//#define NEGATE
void WS2812_Send(uint8_t *rgb, uint16_t count, bool negate, int16_t id)
{
  Pio *pio = HAL_GetPinPio(id);
  uint32_t mask = HAL_GetPinMask(id);
  GlobalInterruptsDisable();
  uint32_t FMR = FlashOptimizationDisable();
  register uint8_t *rgb2 = rgb;
  register int8_t shiver = 1; // Shiver throu +1, -1, 0. This does the RGB to GRB translation (remember Oblivion!)
  register uint16_t count2 = count;
  while (count2--) {
    register uint8_t byte = *(shiver + rgb2++);
    #ifdef NEGATE
      if (byte & 0x80) L else H
      if (byte & 0x40) L else H
      if (byte & 0x20) L else H
      if (byte & 0x10) L else H
      if (byte & 0x08) L else H
      if (byte & 0x04) L else H
      if (byte & 0x02) L else H
      if (byte & 0x01) L else H
    #else
      if (byte & 0x80) H else L
      if (byte & 0x40) H else L
      if (byte & 0x20) H else L
      if (byte & 0x10) H else L
      if (byte & 0x08) H else L
      if (byte & 0x04) H else L
      if (byte & 0x02) H else L
      if (byte & 0x01) H else L
    #endif
    if (shiver == 1)
      shiver = -1;
    else if (shiver == -1)
      shiver = 0;
    else
      shiver = 1;
  }
  #ifdef NEGATE
    H
  #else
    L
  #endif
  FlashOptimizationRestore(FMR);
  GlobalInterruptsEnable();
}
/*---------------------------------------------------------------------------*/

#if 0
// 400ns  850ns (WS2812B)
#define L { \
    pio->PIO_SODR = mask; \ // T0H 
    NOP16(); \
    NOP8(); \
    NOP4(); \
    NOP2(); \
    NOP1(); \
  \
    pio->PIO_CODR = mask; \ // T0L 850ns
    NOP16(); \
    NOP16(); \
    NOP16(); \
    NOP8(); \
    NOP4(); \
    NOP2(); \
    NOP1(); \
  }
/*---------------------------------------------------------------------------*/
// 700ns  600ns
// 800ns  450ns
#define H { \
    pio->PIO_SODR = mask; \ // T1H
    NOP16(); \
    NOP16(); \
    NOP16(); \
    NOP8(); \
    NOP2(); \
    NOP2(); \
  \
    pio->PIO_CODR = mask; \ // T1L
    NOP16(); \
    NOP16(); \
    NOP8(); \
    NOP2(); \
    NOP1(); \
  }
#endif