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
/*---------------------------------------------------------------------------*/
#include "Modifyer.h"
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
/*---------------------------------------------------------------------------
  Modifyer
  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void Modifyer_32(volatile uint32_t *address, uint32_t mask, uint32_t value)
{
  if (address) 
    *address = (*address & ~mask) | (value & mask);
}
/*---------------------------------------------------------------------------*/
void Modifyer_32AutoShift(volatile uint32_t *address, uint32_t mask, uint32_t value)
{
  for (uint32_t i = 1; !(mask & i); i<<= 1) 
    value <<= 1;
  Modifyer_32(address, mask, value);
}
/*---------------------------------------------------------------------------*/
int8_t Modifyer_32ByName(const char *names[], volatile uint32_t *address, uint32_t mask, uint32_t value, const char *name)
{
  int nameLength = strlen(name);
  if (!nameLength) 
    return -3;
  // Calculate inc value
  uint32_t inc = 1; // Start with bit 0
  while (!(mask & inc)) 
    inc <<= 1;
  // Ilterate to names
  for (uint8_t i = 0; names[i]; i++) {
    const char *name2 = names[i];
    if (strlen(name2) && (strncmp(name2, name, nameLength) == 0)) {
      // Name matches,  so modify value
      Modifyer_32(address, mask, value);
      return 1;
    }
    value += inc; // Next value
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
