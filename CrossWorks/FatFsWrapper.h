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
#ifndef FATFSWRAPPER__H
#define FATFSWRAPPER__H
/*===========================================================================
  FatFsWrapper
  ===========================================================================*/
#include "Common.h" /*lint -e537 */
#include <stdio.h>
/*---------------------------------------------------------------------------*/
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get pointer to error message string
 */
char* strerror(int errnum);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Open file
 */
FILE* fopen(const char *filename, const char *mode);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Reopen stream with different file or mode
 */
FILE* freopen(const char *filename, const char *mode, FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Close file
 */
int fclose(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int fprintf(FILE *stream, const char *format, ...);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Flush stream
 */
int fflush(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get character from stream
 */
int getc(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Unget character from stream
 * \note This is not implemented! Always return EOF!
 */
int ungetc(int character, FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Read block of data from stream
 */
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get string from stream
 */
char *fgets(char *str, int num, FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Read formatted data from stream
 */
#if 0
  int fscanf(FILE *stream, const char *format, ...);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Check end-of-file indicator
 */
int feof(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Write block of data to stream
 */
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Reposition stream position indicator
 */
int fseek(FILE *stream, long int offset, int origin);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get current position in stream
 */
long int ftell(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Remove file
 */
int remove(const char *filename);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Rename file
 */
int rename(const char *old_filename, const char *new_filename);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Check error indicator
 */
int ferror(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Clear error indicators
 */
void clearerr(FILE *stream);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Generate temporary filename
 */
char* tmpnam(char * str);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
FILE* tmpfile(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Change stream buffering
 * \note There is no way to change the stream buffering, so return always
 * a failure.
 */
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int system(const char *command);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get environment string
 */
char* getenv(const char* name);
/*---------------------------------------------------------------------------*/
#endif
