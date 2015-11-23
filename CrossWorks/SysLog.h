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
#ifndef SYSLOG_H
#define SYSLOG_H
/*===========================================================================
  Sys log
  ===========================================================================*/
#include "Common.h"
/*---------------------------------------------------------------------------*/
 // facility codes
#define  SYSLOG_KERN      ( 0 << 3)  // kernel messages
#define  SYSLOG_USER      ( 1 << 3)  // random user-level messages
#define  SYSLOG_MAIL      ( 2 << 3)  // mail system
#define  SYSLOG_DAEMON    ( 3 << 3)  // system daemons
#define  SYSLOG_AUTH      ( 4 << 3)  // security/authorization messages
#define  SYSLOG_SYSLOG    ( 5 << 3)  // messages generated internally by syslogd
#define  SYSLOG_LPR       ( 6 << 3)  // line printer subsystem
#define  SYSLOG_NEWS      ( 7 << 3)  // network news subsystem
#define  SYSLOG_UUCP      ( 8 << 3)  // UUCP subsystem
#define  SYSLOG_CRON      ( 9 << 3)  // clock daemon
#define  SYSLOG_AUTHPRIV  (10 << 3)  // security/authorization messages (private)
#define  SYSLOG_PRINTF    (11 << 3)
#define  SYSLOG_PERF      (12 << 3)
#define  SYSLOG_LUAERROR  (13 << 3)
#define  SYSLOG_LOCAL0    (16 << 3)  // reserved for local use
#define  SYSLOG_LOCAL1    (17 << 3)  // reserved for local use
#define  SYSLOG_LOCAL2    (18 << 3)  // reserved for local use
#define  SYSLOG_LOCAL3    (19 << 3)  // reserved for local use
#define  SYSLOG_LOCAL4    (20 << 3)  // reserved for local use
#define  SYSLOG_LOCAL5    (21 << 3)  // reserved for local use
#define  SYSLOG_LOCAL6    (22 << 3)  // reserved for local use
#define  SYSLOG_LOCAL7    (23 << 3)  // reserved for local use
/*---------------------------------------------------------------------------*/
// priorities
#define  SYSLOG_EMERG    0  // system is unusable
#define  SYSLOG_ALERT    1  // action must be taken immediately
#define  SYSLOG_CRIT     2  // critical conditions
#define  SYSLOG_ERR      3  // error conditions
#define  SYSLOG_WARNING  4  // warning conditions
#define  SYSLOG_NOTICE   5  // normal but significant condition
#define  SYSLOG_INFO     6  // informational
#define  SYSLOG_DEBUG    7  // debug-level messages
/*---------------------------------------------------------------------------*/
#define SYSLOG_PORT        514
#define SYSLOG_NOGS_PORT 51514
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void SysLog_Init(int8_t socketNumber);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void SysLog_Config(uint32_t ip, uint16_t sendPort);
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
void SysLog_Send(uint16_t priority, const char *message);
/*---------------------------------------------------------------------------*/
#endif
