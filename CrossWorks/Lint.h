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
#ifndef LINT_H
#define LINT_H
/*===========================================================================
  Lint configuration
  ===========================================================================*/
/*---------------------------------------------------------------------------*/
/*lint -wlib(0) */ /* suppressing warning and informational messages for library includes */
/*lint -format="%( %f line %l column %c %): [%n] %m" */
/*lint -width(0,10) */
/*lint -Dlint=1 */

/*lint -I"C:/Program Files (x86)/Rowley Associates Limited/CrossWorks for ARM 2.3/include" */
/*lint -I"C:/Users/Anwender/AppData/Local/Rowley Associates Limited/CrossWorks for ARM/packages/include" */
/*lint -I"C:/Users/Anwender/AppData/Local/Rowley Associates Limited/CrossWorks for ARM/packages/targets/SAM/CMSIS/Device/ATMEL" */
/*lint -I"C:/Users/Anwender/AppData/Local/Rowley Associates Limited/CrossWorks for ARM/packages/targets/SAM/CMSIS/Device/ATMEL/sam4s/include" */
/*lint -I"C:/Users/Anwender/AppData/Local/Rowley Associates Limited/CrossWorks for ARM/packages/targets/CMSIS_3/CMSIS/Include" */
/*lint -I"C:/Users/Anwender/AppData/Local/Rowley Associates Limited/CrossWorks for ARM/packages/targets/SAM" */
/*lint -I"." */
/*lint -I"./Wiznet" */
/*lint -I"./Wiznet/Ethernet" */

/*lint -I"./FatFs-0-10-c/src" */
/*lint -I"./FatFs-0-10-c/src/option" */
/*lint -I"./lua-5.3.0/src" */


/*lint +libdir("C:\Users\Anwender\AppData\Local\Rowley Associates Limited\CrossWorks for ARM\packages\targets\CMSIS_3\CMSIS\Include") */
/*lint +libdir("./FatFs-0-10-c/src) */
/*lint +libdir("./FatFs-0-10-c/src/option) */
/*lint +libdir("./lua-5.3.0/src) */

/*lint -D__ARM_ARCH_7EM__ */
/*lint -D__CROSSWORKS_ARM */
/*lint -D__CROSSWORKS_MAJOR_VERSION=2 */
/*lint -D__CROSSWORKS_MINOR_VERSION=3 */
/*lint -D__CROSSWORKS_REVISION=3 */
/*lint -DSAM4SA16B=416 */
/*lint -DSAM4S16B=416 */
/*lint -D__TARGET_PROCESSOR=SAM4SA16B */
/*lint -D__SAM4SA16B__= */
/*lint -D__USE_CRYSTAL_OSCILLATOR__= */
/*lint -DUSE_PROCESS_STACK */
/*lint -D__THUMB */
/*lint -D__FLASH_BUILD */
/*lint -DDEBUG */
/*lint -DCHIP_FREQ_XTAL=16000000L */
/*lint -DINITIALIZE_STACK */

/*lint -e793 */ /* ANSI/ISO limit of 4095 exceeded: No limit here! */
/*lint -e830 */ /* local declarator not referenced */
/*lint -e752 */ /* local declarator not referenced */
/*lint -e750 */ /* local macro not referenced */
/*lint -e754 */ /* local structure member not referenced */
/*lint -e756 */ /* global typedef not referenced */
/*lint -e755 */ /* global macro not referenced */
/*lint -e758 */ /* global struct not referenced */
/*lint -e768 */ /* global struct member not referenced */
/*---------------------------------------------------------------------------*/
#if lint 
  void debug_printf(char*,...);
  int abs(int);
  void NOP(void);
  
/*
  #define _SAM4S_SERIES
  #define __TARGET_PROCESSOR 416
  #define __SAM4S16B__
  #define SAM4S16B 416
  #define CHIP_FREQ_XTAL 16000000UL
*/  
#else
  #define NOP(void) _nop_()
#endif

/*---------------------------------------------------------------------------*/
#endif

