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
#include "Memory.h"
#include <string.h>
#include "HAL/HAL.h"
#include "../Wiznet/Ethernet/wizchip_conf.h"
#include "../Wiznet/Ethernet/socket.h"
#ifdef LUA52
  #include "lua-5.2.3/src/lua.h"
  #include "lua-5.2.3/src/lualib.h"
  #include "lua-5.2.3/src/lauxlib.h"
#endif
#ifdef LUA53
  #include "lua-5.3.0/src/lua.h"
  #include "lua-5.3.0/src/lualib.h"
  #include "lua-5.3.0/src/lauxlib.h"
#endif
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
  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//#define MEMORY_GUARDS
//#define MEMORY_GUARD_PRINT
//#define MEMORY_TRACKER_SIZE 2000
//#define MEMORY_TRACKER_PRINT
//#define MEMORY_TRACKER_REMOTE
/*---------------------------------------------------------------------------*/
struct {
  lua_State *L;
  struct {
    size_t size;
    char *file;
    uint16_t line;
  } last, max;
  #if MEMORY_TRACKER_SIZE > 0
    void *tracker[MEMORY_TRACKER_SIZE];
  #endif
  #ifdef MEMORY_TRACKER_REMOTE
    int8_t socketNumber;
    uint32_t ip;
    struct {
      uint32_t count;
      char type;
      char pad1;
      uint16_t pad2;
      uint32_t address1;
      uint32_t address2;
      uint32_t size1;
      uint32_t size2;
      uint32_t line;
      char file[128];
    } buffer;
  #endif
} memory;
/*---------------------------------------------------------------------------*/
#if MEMORY_TRACKER_SIZE > 0
  static void Dump(void)
  {
    DEBUG("\nDump tracked memory\n");
    for (uint16_t i = 0; i < MEMORY_TRACKER_SIZE; i++)
      if (memory.tracker[i]) {
        uint32_t *p = memory.tracker[i];
        #ifdef MEMORY_GUARDS
          size_t size = p[0] & 0xFFFF;
          DEBUG("%05u: %08X %u %08X %08X\n", i, p, size, p[1], p[(size / 4) - 1]);
        #else
          DEBUG("%05u: %08X\n", i, p);
        #endif
      }
    DEBUG("\nDump tracked memory done!\n");
  }
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Build the wall
 * \param p Pointer to the wall
 * \param size Size of the complete memory including the wall
 * \return Pointer to the inner memory
 */
#ifdef MEMORY_GUARDS
  static void* BuildWall(uint32_t *p, size_t size)
  {
    #ifdef MEMORY_GUARD_PRINT
      DEBUG("+ %08X %u\n", p, size);
    #endif

    #if MEMORY_TRACKER_SIZE > 0
      for (uint16_t i = 0; i < MEMORY_TRACKER_SIZE; i++)
        if (!memory.tracker[i]) {
          memory.tracker[i] = p;
          break;
        }
    #endif

    p[0] = size | (size << 16);
    p[1] = 0xDEADBEEF;
    p[(size / 4) - 1] = 0xBEEFDEAD;
    return p + 2;
  }
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Check the wall
 * \param p Pointer to the inner memory
 * \param exprectedSize Size from the previos allocation or zero
 * \return Pointer to the wall
 */
#ifdef MEMORY_GUARDS
  static void* CheckWall(uint32_t *p, size_t exprectedSize)
  {
    if (!p) return;
    p -= 2;
    size_t size = p[0] & 0xFFFF;
    #ifdef MEMORY_GUARD_PRINT
      DEBUG("- %08X %u\n", p, size);
    #endif

    if (exprectedSize) {
      exprectedSize = (exprectedSize + 3) / 4 * 4 + 12;
      if (size != exprectedSize) {
        DEBUG("# size mismatch %u != %u\n", size, exprectedSize);
        #if MEMORY_TRACKER_SIZE > 0
          Dump();
        #endif
      }
    }

    #if MEMORY_TRACKER_SIZE > 0
      bool found = false;
      for (uint16_t i = 0; i < MEMORY_TRACKER_SIZE; i++)
        if (memory.tracker[i] == p) {
          memory.tracker[i] = 0;
          found = true;
          break;
        }
      if (!found) {
        DEBUG("# Free without alloc 0x%08X[%u]\n", p, exprectedSize);
        #if MEMORY_TRACKER_SIZE > 0
          Dump();
        #endif
      }
    #endif

    if ((size != (p[0] >> 16)) || (p[1] != 0xDEADBEEF) || (p[(size / 4) - 1] != 0xBEEFDEAD)) {
      DEBUG("# Memory currupt %08X %08X\n", p[1], p[(size / 4) - 1]);
      #if MEMORY_TRACKER_SIZE > 0
        Dump();
      #endif
    }
    return p;
  }
#endif
/*---------------------------------------------------------------------------*/
static void* Malloc(size_t size, char *file, int line)
{
  memory.last.size = size;
  memory.last.file = file;
  memory.last.line = line;
  if (size > memory.max.size) {
    memory.max.size = size;
    memory.max.file = file;
    memory.max.line = line;
  }

  #ifdef MEMORY_GUARDS
    size = (size + 3) / 4 * 4 + 3 * 4;
    if (size > 0xFFFF) {
      DEBUG("# Memory > 64k\n");
      __asm__("BKPT #01");
      int i = 0;
    }
    void *p = malloc(size);
    if (!p) return 0;
    #ifdef MEMORY_GUARDS
      p = BuildWall(p, size);
    #endif
    return p;
  #else
    void *p = malloc(size);
  #endif
  #ifdef MEMORY_TRACKER_REMOTE
    if (memory.socketNumber >= 0) {
      ++memory.buffer.count;
      memory.buffer.type = 'M';
      memory.buffer.address1 = (uint32_t) p;
      memory.buffer.address2 = 0;
      memory.buffer.size1 = size;
      memory.buffer.size2 = 0;
      memory.buffer.line = line;
      strcpy(memory.buffer.file, file);
      sendto(memory.socketNumber, (void*) &memory.buffer, sizeof(memory.buffer), (uint8_t*) &memory.ip, 44445);
    }
  #endif
  if (!p) {
    //__asm__("BKPT #01");
    int i = 0;
    DEBUG("A:%08X %u %s:%u\n", p, size, file, line);
  }
  #ifdef MEMORY_TRACKER_PRINT
    DEBUG("A:%08X %u %s:%u\n", p, size, file, line);
  #endif
  return p;
}
/*---------------------------------------------------------------------------*/
static void* Realloc(void *ud, void *oldP, size_t oldSize, size_t newSize)
{
  if (!oldP && !oldSize && !newSize) // Nothing to do
    return 0;

  void *newP;
  #ifdef MEMORY_GUARDS
    newSize = (newSize + 3) / 4 * 4 + 3 * sizeof(uint32_t);
    if (newSize > 0xFFFF)
      DEBUG("# Memory > 64k\n");
    if (oldP) oldP = CheckWall(p, oldSize);
    #ifdef MEMORY_TRACKER_REMOTE
      if (memory.socketNumber >= 0) {
        memory.buffer.address1 =  (uint32_t) oldP;
        memory.buffer.size1 = oldSize;
      }
    #endif

    if (newSize)
      newP = realloc(oldP, newSize);
    else {
      free(oldP);
      newP = 0;
    }
    if (!newP || !newSize) return 0;
    #ifdef MEMORY_GUARDS
      newP = BuildWall(newP, newSize);
    #endif
  #else
    if (newSize) {
      if (!oldP)
        newP = malloc(newSize);
      else
        newP = realloc(oldP, newSize);
    } else {
      free(oldP);
      newP = 0;
    }
  #endif
  #ifdef MEMORY_TRACKER_REMOTE
    if (memory.socketNumber >= 0) {
      ++memory.buffer.count;
      memory.buffer.type = 'R';
      memory.buffer.address1 =  (uint32_t) oldP;
      memory.buffer.size1 = oldSize;
      memory.buffer.address2 = (uint32_t) newP;
      memory.buffer.size2 = newSize;
      memory.buffer.line = 0;
      memory.buffer.file[0] = 0;
      sendto(memory.socketNumber, (void*) &memory.buffer, sizeof(memory.buffer), (uint8_t*) &memory.ip, 44445);
    }
  #endif
  #ifdef MEMORY_TRACKER_PRINT
    DEBUG("R:%08X %u %08X %u\n", oldP, oldSize, newP, newSize);
  #endif
  memory.last.size = newSize;
  memory.last.file = 0;
  memory.last.line = 0;
  if (newSize > memory.max.size) {
    memory.max.size = newSize;
    memory.max.file = 0;
    memory.max.line = 0;
  }
  return newP;
}
/*---------------------------------------------------------------------------*/
static void Free(void *p, size_t size, char *file, int line)
{
  #ifdef MEMORY_TRACKER_PRINT
    DEBUG("F:%08X %u %s:%u\n", p, size, file, line);
  #endif
  #ifdef MEMORY_GUARDS
    if (p)
      p = CheckWall(p, size);
  #endif
  #ifdef MEMORY_TRACKER_REMOTE
    if (memory.socketNumber >= 0) {
      ++memory.buffer.count;
      memory.buffer.type = 'F';
      memory.buffer.address1 = (uint32_t) p;
      memory.buffer.address2 = 0;
      memory.buffer.size1 = size;
      memory.buffer.size2 = 0;
      memory.buffer.line = line;
      strcpy(memory.buffer.file, file);
      sendto(memory.socketNumber, (void*) &memory.buffer, sizeof(memory.buffer), (uint8_t*) &memory.ip, 44445);
    }
  #endif
  free(p);
}
/*---------------------------------------------------------------------------*/
void Memory_Init(void)
{
  memset(&memory, 0, sizeof(memory));
  #ifdef MEMORY_TRACKER_REMOTE
    memory.socketNumber = -1;
  #endif
}
/*---------------------------------------------------------------------------*/
void Memory_StartTracker(int8_t socketNumber, uint32_t ip)
{
  #ifdef MEMORY_TRACKER_REMOTE
    memory.socketNumber = socket(socketNumber, Sn_MR_UDP, 44444, 0x00);
    memory.ip = ip;
  #endif
}
/*---------------------------------------------------------------------------*/
void* __Memory_Allocate(uint32_t size, uint8_t flags, char *file, int line)
{
  void *ptr = Malloc(size, file, line);
  if (!ptr && memory.L) {
    // Alloc fails and we have a Lua VM so collect gabbage and try again
    // lua_gc(memory.L, LUA_GCCOLLECT, 0);
    luaC_fullgc(memory.L, 1);
    ptr = Malloc(size, file, line);
  }
  if ((flags & M_SAVE) && !ptr) 
    HAL_SysFatal(FATAL_OUT_OF_MEMORY);
  if ((flags & M_CLEAR) && ptr) 
    memset(ptr, 0, size);
  return ptr;
}
/*---------------------------------------------------------------------------*/
uchar* Memory_NewString(uchar *s, uint8_t flags)
{
  uchar *s2 = (uchar*) Memory_Allocate(strlen(s) + 1, flags);
  if (s2) 
    strcpy(s2, s);
  return s2;
}
/*---------------------------------------------------------------------------*/
uchar* Memory_NewStringLimited(uchar *s, uint32_t maxLength, uint8_t flags)
{
  uint32_t length = strlen(s);
  if (length > maxLength) length = maxLength;
  uchar *s2 = Memory_Allocate(length + 1, flags);
  if (s2) {
    strncpy(s2, s, length);
    s2[length] = 0;
  }
  return s2;
}
/*---------------------------------------------------------------------------*/
/*
static void* LuaRealloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
  (void) ud;
  return Realloc(ptr, osize, nsize);
/*
  (void) ud;
  (void) osize;
  if (!nsize) {
    Del(ptr);
    free(ptr);
    ptr = 0;
  } else {
    Del(ptr);
    ptr = realloc(ptr, nsize);
    Add(ptr, nsize);
  }
  return ptr;
}
*/
/*---------------------------------------------------------------------------*/
lua_State* Memory_AllocLuaVM(void)
{
  if (memory.L) 
    lua_close(memory.L);
  memory.L = lua_newstate(Realloc, 0);
  return memory.L;
}
/*---------------------------------------------------------------------------*/
void __Memory_Free(void* ptr, uint32_t size, char *file, int line)
{
  //MemoryLock();
  if (ptr)
    Free(ptr, size, file, line);
  //MemoryUnlock();
}
/*---------------------------------------------------------------------------*/
void Memory_FreeLuaVM(void)
{
  if (memory.L) {
    lua_close(memory.L);
    memory.L = 0;
  }
}
/*---------------------------------------------------------------------------*/
void Memory_Dump(void)
{
  printf("Memory\n");
  printf("  Last %u %s:%u\n", memory.max.size, memory.max.file, memory.max.line);
  printf("  Max  %u %s:%u\n", memory.max.size, memory.max.file, memory.max.line);
  #if MEMORY_TRACKER_SIZE > 0
    Dump();
  #endif
}
/*---------------------------------------------------------------------------*/


