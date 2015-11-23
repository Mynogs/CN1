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
#include "FatFsWrapper.h"
#include "FatFs-0-10-c/src/ff.h"
#include "Memory.h"
/*---------------------------------------------------------------------------*/
#undef DEBUG
#ifdef STARTUP_FROM_RESET
  #define DEBUG(...)
#else
  #if 0
    extern int debug_printf(const char *format, ...);
    #define DEBUG debug_printf
  #else
    #define DEBUG(...)
  #endif
#endif
/*---------------------------------------------------------------------------*/
#define STD_IN  0
#define STD_OUT 1
#if 1
  #define STD_ERR STD_OUT
#else
  #define STD_ERR 2
#endif
/*---------------------------------------------------------------------------*/
extern int putchar(int __c);
/*---------------------------------------------------------------------------*/
FILE *stdin  = (FILE*) STD_IN;
FILE *stdout = (FILE*) STD_OUT;
FILE *stderr = (FILE*) STD_ERR;
/*---------------------------------------------------------------------------*/
char* strerror(int errnum)
{
  switch (errnum) {
    case FR_OK :                  return "Succeeded";
    case FR_DISK_ERR :            return "A hard error occurred in the low level disk I/O layer";
    case FR_INT_ERR :             return "Assertion failed";
    case FR_NOT_READY :           return "The physical drive cannot work";
    case FR_NO_FILE :             return "Could not find the file";
    case FR_NO_PATH :             return "Could not find the path";
    case FR_INVALID_NAME :        return "The path name format is invalid";
    case FR_DENIED :              return "Access denied due to prohibited access or directory full";
    case FR_EXIST :               return "Access denied due to prohibited access";
    case FR_INVALID_OBJECT :      return "The file/directory object is invalid";
    case FR_WRITE_PROTECTED :     return "The physical drive is write protected";
    case FR_INVALID_DRIVE :       return "The logical drive number is invalid";
    case FR_NOT_ENABLED :         return "The volume has no work area";
    case FR_NO_FILESYSTEM :       return "There is no valid FAT volume";
    case FR_MKFS_ABORTED :        return "The f_mkfs aborted due to any parameter error";
    case FR_TIMEOUT :             return "Could not get a grant to access the volume within defined period";
    case FR_LOCKED :              return "The operation is rejected according to the file sharing policy";
    case FR_NOT_ENOUGH_CORE :     return "LFN working buffer could not be allocated";
    case FR_TOO_MANY_OPEN_FILES : return "Number of open files > _FS_SHARE";
    case FR_INVALID_PARAMETER :   return "Given parameter is invalid";
  }
  return "Unknown";
}
/*---------------------------------------------------------------------------*/
FILE* fopen(const char *filename, const char *mode)
{
  DEBUG("fopen(%s, %s)\n", filename, mode);
  FIL* fp = Memory_Allocate(sizeof(FIL), 0);
  BYTE modeFlags = 0;
  while (*mode) {
    switch (*mode++) {
      case 'r' : modeFlags |= FA_READ | FA_OPEN_EXISTING; break;
      case 'w' : modeFlags |= FA_WRITE | FA_CREATE_ALWAYS; break;
      // TODO a, +
    }
  }
  FRESULT result = f_open(fp, filename, modeFlags);
  if (result != FR_OK) {
    Memory_Free(fp, sizeof(FIL));
    DEBUG("  fail\n");
    return 0;
  }
  DEBUG("  %08X\n", fp);
  return fp;
}
/*---------------------------------------------------------------------------*/
FILE* freopen(const char *filename, const char *mode, FILE *stream)
{
  DEBUG("freopen(%s, %s)\n", filename, mode);
  fclose(stream);
  return fopen(filename, mode);
}
/*---------------------------------------------------------------------------*/
int fclose(FILE *stream)
{
  DEBUG("fclose(%08X)\n", stream);
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  f_close((FIL*) stream);
  Memory_Free(stream, sizeof(FIL));
  return 0;
}
/*---------------------------------------------------------------------------*/
int fprintf(FILE *stream, const char *format, ...)
{
  return f_printf(stream, format);
}
/*---------------------------------------------------------------------------*/
int fflush(FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return 0;
  return f_sync(stream) == FR_OK ? 0 : EOF;
}
/*---------------------------------------------------------------------------*/
int getc(FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  char c;
  UINT readCount;
  f_read(stream, &c, 1, &readCount);
  return readCount ? c : EOF;
}
/*---------------------------------------------------------------------------*/
int ungetc(int character, FILE *stream)
{
  return EOF;
}
/*---------------------------------------------------------------------------*/
size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  if (!size || !count) return 0;
  UINT readCount;
  f_read(stream, ptr, size * count, &readCount);
  return readCount;
}
/*---------------------------------------------------------------------------*/
char* fgets(char *str, int num, FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return 0;
  return f_gets(str, num, stream);
}
/*---------------------------------------------------------------------------*/
#ifdef LUA52
  int fscanf(FILE *stream, const char *format, ...)
  {
    return 0;
  }
#endif
/*---------------------------------------------------------------------------*/
int feof(FILE *stream)
{
  return ((uint32_t) stream <= STD_ERR) || f_eof((FIL*) stream) ? EOF : 0;
}
/*---------------------------------------------------------------------------*/
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
  if (!size || !count) return 0;
  if ((int) stream == STD_IN)
    return 0;
  if ((int) stream == STD_OUT) {
    uint32_t n = size * count;
    char *ptr2 = (char*) ptr;
    for (uint32_t i = 0; i < n; i++) putchar(*ptr2++);
    return n;
  }
  UINT writeCount;
  f_write(stream, ptr, size * count, &writeCount);
  return writeCount;
}
/*---------------------------------------------------------------------------*/
int fseek(FILE *stream, long int offset, int origin)
{
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  switch (origin) {
    case SEEK_SET : return f_lseek(stream, (DWORD) offset) == FR_OK ? 0 : EOF;
    case SEEK_CUR : return f_lseek(stream, f_tell((FIL*) stream) + offset) == FR_OK ? 0 : EOF;
    case SEEK_END : return f_lseek(stream, f_size((FIL*) stream) + offset) == FR_OK ? 0 : EOF;
  }
  return EOF;
}
/*---------------------------------------------------------------------------*/
long int ftell(FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  return f_tell((FIL*) stream);
}
/*---------------------------------------------------------------------------*/
int remove(const char *filename)
{
  return f_unlink(filename) == FR_OK ? 0 : EOF;
}
/*---------------------------------------------------------------------------*/
int rename(const char *old_filename, const char *new_filename)
{
  return f_rename(old_filename, new_filename) == FR_OK ? 0 : EOF;
}
/*---------------------------------------------------------------------------*/
int ferror(FILE *stream)
{
  if ((uint32_t) stream <= STD_ERR)
    return EOF;
  return stream ? f_error((FIL*) stream) : 0;
}
/*---------------------------------------------------------------------------*/
void clearerr(FILE *stream)
{
  if ((uint32_t) stream <=  STD_ERR)
    f_error((FIL*) stream) = FR_OK;
}
/*---------------------------------------------------------------------------*/
char* tmpnam(char * str)
{
  static char buffer[13];
  uint32_t count = 0;
  snprintf(buffer, sizeof(buffer), "%08u.$$$", ++count);
  return buffer;
  return 0;
}
/*---------------------------------------------------------------------------*/
FILE *tmpfile(void)
{
  return fopen(tmpnam(0), "rw");
}
/*---------------------------------------------------------------------------*/
int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
  return EOF;
}
/*---------------------------------------------------------------------------*/
int system(const char *command)
{
  return EOF;
}
/*---------------------------------------------------------------------------*/
char* getenv(const char* name)
{
  /*
  if (strcmp(name,"LUA_PATH") == 0) {
    static char buffer[64];
    TCHAR* drive[2];
    if (f_getcwd(drive, sizeof(drive)) != FR_OK) return 0;
    snprintf(buffer, sizeof(buffer), "%s?;%s?.lua;%scn/?;%scn/?.lua", drive, drive, drive);
    return buffer;
  }*/
  if (strcmp(name,"LUA_PATH") == 0)
    return
      "?.lua;"
      "?;"
      "cn/?.lua;"
      "cn/?"
      ;

  if (strcmp(name,"LUA_CPATH") == 0) return ""; //"?.elf";
  return 0;
}
/*---------------------------------------------------------------------------*/










