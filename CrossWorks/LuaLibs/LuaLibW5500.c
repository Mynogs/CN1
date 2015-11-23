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
#include "LuaLibW5500.h"
#include "../Wiznet/Ethernet/wizchip_conf.h"
#include "../Wiznet/Ethernet/socket.h"
#include "../Common.h"
#include "../HAL/HAL.h"
#include "../HAL/HALW5500.h"
#include "lua-5.3.0/src/lua.h"
#include "lua-5.3.0/src/lualib.h"
#include "lua-5.3.0/src/lauxlib.h"
#include "../lua-5.3.0/src/lstate.h" // ???????
#include "../SysLog.h"
#include "LuaLibUtils.h"
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
#define EVENT_FLAG_CON       Sn_IR_CON
#define EVENT_FLAG_DISCON    Sn_IR_DISCON
#define EVENT_FLAG_RECV      Sn_IR_RECV
#define EVENT_FLAG_TIMEOUT   Sn_IR_TIMEOUT
#define EVENT_FLAG_SENDOK    Sn_IR_SENDOK
#define EVENT_FLAG_LINK_DOWN (1 << 8)
#define EVENT_FLAG_LINK_UP   (1 << 9)
/*---------------------------------------------------------------------------*/
typedef enum SocketType {
  tFree = 0,            // Socket is not used
  tTCP = Sn_MR_TCP,     // (== 1) TCP socket
  tUDP = Sn_MR_UDP,     // (== 2) UDP socket
  tRAW = Sn_MR_MACRAW,  // (== 3) MACRAW socket
  tAlien                // Use from someone other, type unknown
} SocketType;
/*---------------------------------------------------------------------------*/
typedef struct Socket {
  SocketType type;      // Type of the cocket
  uint16_t port;        // Assigned port
  uint32_t ip;          // Assigned ip in the form ll.lh.hl.hh. 0 means sever
  int handlerReference; // Reference to handler function or handler thread
  lua_State *L;         // If handler is a thread this points to thread context. Null means handler is a function
} Socket;
/*---------------------------------------------------------------------------*/
typedef struct W5500 {
  uint32_t sentinal1;
  bool active;            // Network is up
  bool link;
  Socket sockets[_WIZCHIP_SOCK_NUM_];
  uint32_t sentinal2;
} W5500;
W5500 w5500;
/*---------------------------------------------------------------------------*/
// Utils
/*---------------------------------------------------------------------------*/
uint16_t GetLinkFlag(void)
{
  return HALW5500_IsLinkUp() ? EVENT_FLAG_LINK_UP : EVENT_FLAG_LINK_DOWN;
}
/*---------------------------------------------------------------------------*/
struct Socket* GetSocket(uint8_t socketNumber)
{
  if ((socketNumber >= _WIZCHIP_SOCK_NUM_) || (w5500.sentinal1 != 0xBEEFBEEF) || (w5500.sentinal2 != 0xBEEFBEEF)) 
    return 0;
  return &w5500.sockets[socketNumber];
}
/*---------------------------------------------------------------------------*/
static void PushHandler5Parameter(lua_State *L, uint8_t socketNumber, uint32_t ip, uint16_t port, uint16_t events, uint8_t status)
{
  lua_pushinteger(L, socketNumber);
  lua_pushinteger(L, ip);
  lua_pushinteger(L, port);
  lua_pushinteger(L, events);
  lua_pushinteger(L, status);
}
/*---------------------------------------------------------------------------*/
static void PushHandler2Parameter(lua_State *L, uint16_t events, uint8_t status)
{
  lua_pushinteger(L, events);
  lua_pushinteger(L, status);
}
/*---------------------------------------------------------------------------*/
static char* StatusToString(uint8_t status)
{
  switch (status) {
    case SOCK_CLOSED      : return "CLOSED";
    case SOCK_INIT        : return "INIT";
    case SOCK_LISTEN      : return "LISTEN";
    case SOCK_SYNSENT     : return "SYNSENT";
    case SOCK_SYNRECV     : return "SYNRECV";
    case SOCK_ESTABLISHED : return "ESTABLISHED";
    case SOCK_FIN_WAIT    : return "FIN_WAIT";
    case SOCK_CLOSING     : return "CLOSING";
    case SOCK_TIME_WAIT   : return "TIME_WAIT";
    case SOCK_CLOSE_WAIT  : return "CLOSE_WAIT";
    case SOCK_LAST_ACK    : return "LAST_ACK";
    case SOCK_UDP         : return "UDP";
    case SOCK_MACRAW      : return "MACRAW";
  }
  return "???";
}
/*---------------------------------------------------------------------------*/
static char* EventsToString(char *s, uint8_t event)
{
  void Add(const char *s2)
  {
    if (s[0]) 
      strcat(s, "|");
    strcat(s, s2);
  }

  s[0] = 0;
  if (event & EVENT_FLAG_CON)       
    Add("CON");
  if (event & EVENT_FLAG_DISCON)    
    Add("DISCON");
  if (event & EVENT_FLAG_RECV)      
    Add("RECV");
  if (event & EVENT_FLAG_TIMEOUT)   
    Add("TIMEOUT");
  if (event & EVENT_FLAG_SENDOK)    
    Add("SENDOK");
  if (event & EVENT_FLAG_LINK_DOWN) 
    Add("LINK_DOWN");
  if (event & EVENT_FLAG_LINK_UP)   
    Add("LINK_UP");
  return s;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Helper to return an LuaUtilsError.
 * Function returns two things: nil,  text
 * \param L The Lua VM
 * \param code Wiznet LuaUtilsError code
 */
static int8_t ErrorCode(lua_State *L, int code)
{
  lua_pushnil(L);
  char *s = "Unknown"; // TODO Kürzer machen!
  switch (code) {
    case SOCKERR_SOCKNUM    : s = "Invalid socket number"; break;
    case SOCKERR_SOCKOPT    : s = "Invalid socket option"; break;
    case SOCKERR_SOCKINIT   : s = "Socket is not initialized"; break;
    case SOCKERR_SOCKCLOSED : s = "Socket unexpectedly closed"; break;
    case SOCKERR_SOCKMODE   : s = "Invalid socket mode for socket operation (not open?)"; break;
    case SOCKERR_SOCKFLAG   : s = "Invalid socket flag"; break;
    case SOCKERR_SOCKSTATUS : s = "Invalid socket status for socket operation"; break;
    case SOCKERR_ARG        : s = "Invalid argrument"; break;
    case SOCKERR_PORTZERO   : s = "Port number is zero"; break;
    case SOCKERR_IPINVALID  : s = "Invalid IP address"; break;
    case SOCKERR_TIMEOUT    : s = "Timeout occurred"; break;
    case SOCKERR_DATALEN    : s = "Data length is zero or greater than buffer max size"; break;
    case SOCKERR_BUFFER     : s = "Socket buffer is not enough for data communication"; break;
  }
  return LuaUtils_Error(L, s);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Handle socket
 * If the handler is a function, simple call it. If the handler is a thread
 * resum the thread.
 */
static bool HandelSocket(lua_State *L, uint8_t socketNumber, uint16_t events, uint8_t status)
{
  Socket *s = GetSocket(socketNumber);
  if (s->type < tAlien) {
    if (s->L) {                                                                         // Has handler a context then the handler is a thread
      if (lua_status(s->L) != LUA_YIELD)                                                // If the thread is in yield state?
        return false;                                                                   // No, can't handle so return false
      PushHandler2Parameter(s->L, events, status);                                      // Resume the thread with two parameters: event and status
      int result = lua_resume(s->L, 0, 2);                                              // Resume th thread
      if (result == LUA_OK)                                                             // It the result is LUA_OK?
        LuaLibW5500_FreeSocket(socketNumber);                                           // Thread was ending, so free the socket
      else if (result > LUA_YIELD)                                                      // If the result is an error
        luaL_error(L, "Error in socket handler thread:\n\t%s", lua_tostring(s->L, -1)); // Throw the error
    } else {                                                                            // Handler is a function
      lua_pushlightuserdata(L, (void*) s);                                              // Socket address is the index to the regirtry
      //LuaUtils_DumpStack(L);
      lua_gettable(L, LUA_REGISTRYINDEX);                                               // Get the function reference
      //LuaUtils_DumpStack(L);
      PushHandler5Parameter(L, socketNumber, s->ip, s->port, events, status);           // Call the function with 5 parameters: socketNumber, ip, port, event and status
      //LuaUtils_DumpStack(L);
      lua_call(L, 5, 0);                                                                // Call the function
    }
  }
  return true;
}
/*---------------------------------------------------------------------------*/
// Exported functions
/*---------------------------------------------------------------------------*/
void LuaLibW5500_Init(void)
{
  DEBUG("LuaLibW5500_Init\n");
  memset(&w5500, 0, sizeof(w5500));
  w5500.sentinal1 = w5500.sentinal2 = 0xBEEFBEEF;
}
/*---------------------------------------------------------------------------*/
int16_t LuaLibW5500_GetFreeSocket(void)
{
  DEBUG("LuaLibW5500_GetFreeSocket: ");
  for (int8_t socketNumber = _WIZCHIP_SOCK_NUM_ - 1; socketNumber >= 0 ;socketNumber--)
    if (w5500.sockets[socketNumber].type == tFree) {
      w5500.sockets[socketNumber].type = tAlien;
      DEBUG("%d\n", socketNumber);
      return socketNumber;
    }
  DEBUG("Fail\n");
  return RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
int16_t LuaLibW5500_GetSocket0(void)
{
  DEBUG("LuaLibW5500_GetSocket0: ");
  if (w5500.sockets[0].type == tFree) {
    w5500.sockets[0].type = tAlien;
    DEBUG("Ok\n");
    return 0;
  }
  DEBUG("Fail\n");
  return RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
void LuaLibW5500_FreeSocket(uint8_t socketNumber)
{
  DEBUG("LuaLibW5500_FreeSocket(%d)\n", socketNumber);
  Socket *s = GetSocket(socketNumber);
  if (s->type != tFree)
    close(socketNumber);
  s->L = 0;
  s->type = tFree;
}
/*---------------------------------------------------------------------------*/
uint16_t ChangeEndian16(uint16_t v)
{
  return (v >> 8) | (v << 8);
}
/*-----------------------------------------------------------------------------*/
uint32_t ChangeEndian32(uint32_t v)
{
  return ((uint32_t) ChangeEndian16(v) << 16) | ChangeEndian16(v >> 16);
}
/*-----------------------------------------------------------------------------*/
static bool_t IsSpecialIP(uint32_t ip)
{
  return !ip && !~ip;
}
/*---------------------------------------------------------------------------*/
bool StringToIP(const char *s, uint32_t *ip)
{
  if (strcmp(s, "BROADCAST") == 0) return *ip = 0xFFFFFFFF, true;
  if (strcmp(s, "NULL") == 0) return *ip = 0x00000000, true;
  bool valid = false;
  if (strlen(s) >= 7) {
    int a = atoi(s);
    s = strchr(s, '.');
    if (s++) {
      int b = atoi(s);
      s = strchr(s, '.');
      if (s++) {
        int c = atoi(s);
        s = strchr(s, '.');
        if (s++) {
          int d = atoi(s);
          if ((a >= 0) && (a <= 255) && (b >= 0) && (b <= 255) && (c >= 0) && (c <= 255) && (d >= 0) && (d <= 255)) {
            *ip = (d << 24) | (c << 16) | (b << 8) | a;
            return true;
          }
        }
      }
    }
  }
  return false;
}
/*---------------------------------------------------------------------------*/
void IPToString(char *s, size_t size, uint32_t ip)
{
  snprintf(s, size, "%u.%u.%u.%u", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
}
/*---------------------------------------------------------------------------*/
void MACToSTring(char *s, size_t size, uint8_t *mac)
{
  for (uint8_t i = 0; i <= 5;i++) {
    snprintf(s, size, "%02X", *mac++);
    s += 2;
  }
}
/*---------------------------------------------------------------------------*/
// Lua functions
/*---------------------------------------------------------------------------*/


  //int8_t HALW5500_Config(const char *config, uint8_t configNumber);




LIB_FUNCTION(LuaSetIP, "true|E.ip,mask")
{
  DEBUG("NetSetIP\n");
  w5500.active = false;
  LuaUtils_CheckParameter1(L, 2, LuaSetIPUsage);
  uint32_t ip = luaL_checkinteger(L, 1);
  uint32_t mask = luaL_checkinteger(L, 2);
  if (IsSpecialIP(ip))
    return ErrorCode(L, SOCKERR_IPINVALID);
  HALW5500_SetIP(ip, mask);
  (void) HALW5500_WaitForLinkUp(3); //// Notig????????????????
  w5500.active = true;
  lua_pushboolean(L, w5500.active);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSetGateway, "true|E.gateway")
{
  DEBUG("NetSetGateway\n");
  LuaUtils_CheckParameter1(L, 1, LuaSetGatewayUsage);
  uint32_t gateway = luaL_checkinteger(L, 1);
  if (IsSpecialIP(gateway))
    return ErrorCode(L, SOCKERR_IPINVALID);
  HALW5500_SetGateway(gateway);
  lua_pushboolean(L, w5500.active);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaServe, ".")
{
  for (int8_t socketNumber = _WIZCHIP_SOCK_NUM_ - 1; socketNumber >= 0 ;socketNumber--) { // Ilterate thow all sockets
    SocketType type = w5500.sockets[socketNumber].type;
    if ((type > tFree) && (type < tAlien)) {                                              // Is the socket handled by Lua
      uint16_t events = getSn_IR(socketNumber);                                           // Get the events. TODO do it with interrups
      static uint8_t div = 0;
      if (!div++) {                                                                       // Every 256s call test the link status
        bool link = HALW5500_IsLinkUp();
        if (w5500.link != link) {                                                         // If link status change
          w5500.link = link;                                                              // Remember new link state
          events &= ~(EVENT_FLAG_LINK_UP | EVENT_FLAG_LINK_DOWN);                         // Clear the link up and down flags
          events |= link ? EVENT_FLAG_LINK_UP : EVENT_FLAG_LINK_DOWN;                     // Set new link status as up or down flag
        }
      }
      if (events) {                                                                       // If something happens
        DEBUG("---- %u %u\n", socketNumber, events);
        uint8_t status = getSn_SR(socketNumber);                                          // Get the status flags
        if (HandelSocket(L, socketNumber, events, status))                                // Call the handler
          setSn_IR(socketNumber, events);                                                 // If handled clear the status flags
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaStringToIP, "ip|E.S")
{
  LuaUtils_CheckParameter1(L, 1, LuaStringToIPUsage);
  const char *s = luaL_checkstring(L, 1);
  uint32_t ip;
  if (!StringToIP(s, &ip))
    return LuaUtils_Error(L, "Malformed ip address");
  lua_pushinteger(L, ip);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaIPToString, "S.ip")
{
  LuaUtils_CheckParameter1(L, 1, LuaIPToStringUsage);
  uint32_t ip = luaL_checkinteger(L, 1);
  char s[12];
  IPToString(s, sizeof(s), ip);
  lua_pushstring(L, s);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaStatusToString, "S.status")
{
  LuaUtils_CheckParameter1(L, 1, LuaStatusToStringUsage);
  lua_pushstring(L, StatusToString(luaL_checkinteger(L, 1)));
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaEventsToString, "S.events")
{
  LuaUtils_CheckParameter1(L, 1, LuaEventsToStringUsage);
  static char s[32];
  lua_pushstring(L, EventsToString(s, luaL_checkinteger(L, 1)));
  return 1;
}
/*---------------------------------------------------------------------------*/
// socket | nil, error = w5500.create("TCP", port, function | thread)      TCP server listening on 'port'
// socket | nil, error = w5500.create("TCP", ip, port, function | thread)  TCP client connect to 'port'
// socket | nil, error = w5500.create("UDP", port, function | thread)      UDP receiving on 'port'
// socket | nil, error = w5500.create("UDP", ip, port, function | thread)  UDP receiving on 'port' and default send 'ip'
LIB_FUNCTION(LuaCreate, "socket|E.'TCP'|'UDP'[,ip],port,F|T")
{
  int n = LuaUtils_CheckParameter2(L, 3, 4, LuaCreateUsage);
  int8_t socketNumber = LuaLibW5500_GetFreeSocket();
  if (socketNumber < 0)
    return LuaUtils_Error(L, "No more free sockets");
  Socket *s = GetSocket(socketNumber);
  const char *type = luaL_checkstring(L, 1);
  if (strcmp(type, "UDP") == 0)
    s->type = tUDP;
  else if (strcmp(type, "TCP") == 0)
    s->type = tTCP;
  else
    return LuaUtils_Error(L, "type must be UDP or TCP");
  s->ip = n == 4 ? luaL_checkinteger(L, 2) : 0;
  s->port = luaL_checkinteger(L, n - 1);
  {
    int type = lua_type(L, n);                                        // Get the type of the last parameter
    if ((type != LUA_TFUNCTION) && (type != LUA_TTHREAD))             // If the type is function or thread?
      luaL_error(L, "handler function must be a function or thread"); // Not, throw error
    lua_pushlightuserdata(L, (void*) s);                              // Socket address is the index to the registry
    lua_pushvalue(L, n);                                              // Push the function or thread to the top of the stack
    lua_settable(L, LUA_REGISTRYINDEX);                               // Store it the registry
    if (type == LUA_TTHREAD) {                                        // It was a thread start it for the first time
      uint16_t events = getSn_IR(socketNumber);
      s->L = lua_tothread(L, n);                                      // Get the thread
      PushHandler5Parameter(                                          // Resume the thread with 5 parameters:
        s->L,                                                         //   The VM for the parameters
        socketNumber,                                                 //   socketNumber
        s->ip,                                                        //   ip
        s->port,                                                      //   port
        events | GetLinkFlag(),                                       //   event, this is the first call FLAG_LINK_* is always included
        getSn_SR(socketNumber)                                        //   staus
      );
      lua_resume(s->L, L, 5);                                         // Start the thread via resume
      setSn_IR(socketNumber, events);
    } else                                                            // It was a function do nothing
      s->L = 0;                                                       // Thrad VM is null
  }
  lua_pushinteger(L, socketNumber);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaOpen, "true|E.socket")
{
  LuaUtils_CheckParameter1(L, 1, LuaOpenUsage);
  int socketNumber = luaL_checkinteger(L, 1);
  Socket *s = GetSocket(socketNumber);
  int16_t result = socket(socketNumber, s->type, s->port, 0x00);
  if (result != socketNumber)
    return ErrorCode(L, result);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaListen, "true|E.socket")
{
  LuaUtils_CheckParameter1(L, 1, LuaListenUsage);
  int socketNumber = luaL_checkinteger(L, 1);
  int16_t result = listen(socketNumber);
  if (result < 0) 
    return ErrorCode(L, result);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaConnect, "true|E.socket[,ip[,port]]")
{
  int n = LuaUtils_CheckParameter2(L, 2, 3, LuaConnectUsage);
  int socketNumber = luaL_checkinteger(L, 1);
  Socket *s = GetSocket(socketNumber);
  uint32_t ip = n >= 2 ? luaL_checkinteger(L, 2) : s->ip;
  uint16_t port = n >= 3 ? luaL_checkinteger(L, 3) : s->port;
  int16_t result = connect(socketNumber, (uint8_t*) ip, port);
  if (result < 0) 
    return ErrorCode(L, result);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaDisconnect, "true|E.socket")
{
  LuaUtils_CheckParameter1(L, 1, LuaDisconnectUsage);
  int socketNumber = luaL_checkinteger(L, 1);
  int16_t result = disconnect(socketNumber);
  if (result < 0) 
    return ErrorCode(L, result);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSend, "true|E.socket,packet/true|E.socket,packet[,ip[,port]]")
{
  int n = lua_gettop(L);
  int socketNumber = luaL_checkinteger(L, 1);
  Socket *s = GetSocket(socketNumber);
  int16_t result;
  if (s->type == tTCP) {
    LuaUtils_CheckParameter1(L, 2, "true|E.socket,packet");
    const char *packet = luaL_checkstring(L, 2);
    uint16_t size = lua_rawlen(L, 2);
    result = send(socketNumber, (char*) packet, size);
  } else {
    LuaUtils_CheckParameter2(L, 2, 4, "true|E.socket,packet[,ip[,port]]");
    const char *packet = luaL_checkstring(L, 2);
    uint16_t size = lua_rawlen(L, 2);
    uint32_t ip = n >= 3 ? luaL_checkinteger(L, 3) : s->ip;
    uint16_t port = n >= 4 ? luaL_checkinteger(L, 4) : s->port;
    result = sendto(socketNumber, (char*) packet, size, (uint8_t*) &ip, port);
  }
  if (result < 0) 
    return ErrorCode(L, result);
  lua_pushboolean(L, true);
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaRecv, "packet|E.socket/packet,fromip,fromport|E.socket")
{
  LuaUtils_CheckParameter1(L, 1, LuaRecvUsage);
  int socketNumber = luaL_checkinteger(L, 1);
  Socket *s = GetSocket(socketNumber);
  uint16_t size = getSn_RX_RSR(socketNumber);
  if (size) {
    int16_t result;
    if (s->type == tTCP) {
      luaL_Buffer buffer;
      char *packet = luaL_buffinitsize(L, &buffer, size);
      int16_t result = recv(socketNumber, packet, size);
      if (result < 0) {
        // TODO Free buffer
        return ErrorCode(L, result);
      }
      luaL_pushresultsize(&buffer, size);
      return 1;
    } else {
      size -= 8; // Remove the size of the UDP header. Why???
      luaL_Buffer buffer;
      char *packet = luaL_buffinitsize(L, &buffer, size);
      if (!packet) {
        int i = 0;
      }
      uint32_t ip;
      uint16_t port;
      int16_t result = recvfrom(socketNumber, packet, size, (uint8_t*) &ip, &port);
      if (result < 0) {
        luaL_pushresultsize(&buffer, 0);
        lua_pop(L, 1);
        return ErrorCode(L, result);
      }
      luaL_pushresultsize(&buffer, result);
      lua_pushinteger(L, ip);
      lua_pushinteger(L, port);
      return 3;
    }
  }
  lua_pushstring(L, "");
  return 1;
}
/*---------------------------------------------------------------------------*/
LIB_FUNCTION(LuaSysLog, ".priority,message")
{
  uint16_t priority = luaL_checkinteger(L, 1);
  const char *message = luaL_checkstring(L, 2);
  SysLog_Send(priority, message);
  return 0;
}
/*---------------------------------------------------------------------------*/
LIB_BEGIN(Lib, "w5500")
  LIB_ITEM_FUNCTION("setIP", LuaSetIP)
  LIB_ITEM_FUNCTION("setGateway", LuaSetGateway)
  LIB_ITEM_FUNCTION("serve", LuaServe)
  LIB_ITEM_FUNCTION("stringToIP", LuaStringToIP)
  LIB_ITEM_FUNCTION("ipToString", LuaIPToString)
  LIB_ITEM_FUNCTION("statusToString", LuaStatusToString)
  LIB_ITEM_FUNCTION("eventsToString", LuaEventsToString)
  LIB_ITEM_FUNCTION("create", LuaCreate)
  LIB_ITEM_FUNCTION("open", LuaOpen)
  LIB_ITEM_FUNCTION("listen", LuaListen)
  LIB_ITEM_FUNCTION("connect", LuaConnect)
  LIB_ITEM_FUNCTION("disconnect", LuaDisconnect)
  LIB_ITEM_FUNCTION("send", LuaSend)
  LIB_ITEM_FUNCTION("recv", LuaRecv)
  LIB_ITEM_FUNCTION("sysLog", LuaSysLog)
  LIB_ITEM_INTEGER("CON", EVENT_FLAG_CON)
  LIB_ITEM_INTEGER("DISCON", EVENT_FLAG_DISCON)
  LIB_ITEM_INTEGER("RECV", EVENT_FLAG_RECV)
  LIB_ITEM_INTEGER("TIMEOUT", EVENT_FLAG_TIMEOUT)
  LIB_ITEM_INTEGER("SENDOK", EVENT_FLAG_SENDOK)
  LIB_ITEM_INTEGER("LINKDOWN", EVENT_FLAG_LINK_DOWN)
  LIB_ITEM_INTEGER("LINKUP", EVENT_FLAG_LINK_UP)
LIB_END
/*---------------------------------------------------------------------------*/
void LuaLibW5500_Register(lua_State *L)
{
  DEBUG("LuaLibW5500_Register\n");

  // Cleanup socket from previus run
  // If the socket has a Lua VM then this socket was opened from Lua. So free this socket.
  // All other sockets are used from a c module. Don't tought them!
  for (int8_t socketNumber = _WIZCHIP_SOCK_NUM_ - 1; socketNumber >= 0 ;socketNumber--) {
    Socket *s = GetSocket(socketNumber);
    DEBUG("[%d] %d L:0x%08X\n", socketNumber, s->type, s->L);
    if (s->handlerReference)
      LuaLibW5500_FreeSocket(socketNumber);
  }
  w5500.link = HALW5500_IsLinkUp();

  LuaUtils_Register(L, "sys", "w5500", Lib);
}
/*---------------------------------------------------------------------------*/
