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
#include "ATSHA204.h"
#include "Memory.h"
#include "SHA204/src/sha204_comm_marshaling.h"
#include "SHA204/src/sha204_lib_return_codes.h"
#include "SHA204/src/sha204_physical.h"
#include "SHA204/src/sha204_examples.h"
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
  ATSHA204
  ---------------------------------------------------------------------------*/
#undef SHA204_CLIENT_ADDRESS
#define SHA204_CLIENT_ADDRESS 0


uint8_t sha204e_lock_data_zone(uint8_t device_id)
{
  uint8_t ret_code;
  uint8_t config_data[SHA204_DATA_SIZE];
  uint8_t crc_array[SHA204_CRC_SIZE];
  uint16_t crc;
  uint8_t command[LOCK_COUNT];
  uint8_t response[LOCK_RSP_SIZE];

  sha204p_sleep();

  ret_code = sha204c_wakeup(response);
  ret_code = sha204m_lock(command, response, SHA204_ZONE_DATA | LOCK_ZONE_NO_CRC, 0);

  return ret_code;
}


/*---------------------------------------------------------------------------*/
bool ATSHA204_Init(void)
{
}
/*---------------------------------------------------------------------------*/
bool ATSHA204_GetSerialNumber(uint8_t *data9)
{
/*
  HALATSHA204Wake();
  uint8 b1[2] = {0,0x88};
  HALSHA204Send(2,b1);
  uint8 b2[4];
  HALSHA204Recv(4,b2);
*/
  uint8_t config[SHA204_CONFIG_SIZE];
  uint8_t result = sha204e_read_config_zone(SHA204_CLIENT_ADDRESS, config);
  if (result) return false;
  memcpy(data9, &config[0], 4);
  memcpy(data9 + 4, &config[8], 5);

  #if 0
  DEBUG("\nConfig Zone:\n");
  DEBUG("RevNum          0x%08X\n", *((uint32*) &config[4]));
  DEBUG("Reseverd (0x55) 0x%02X\n", config[13]);
  DEBUG("I2C enabled     %u\n", config[14]);
  DEBUG("    address     %u\n", config[16]);
  DEBUG("OTP mode        0x%02u\n", config[18]);
  DEBUG("Selector mode   0x%02u\n", config[19]);
  {
    DEBUG("Slot config\n");
    uint16 *ptr = (uint16*) &config[20];
    for (uint8 i = 0; i <= 15; i++) {
      uint16 x = *ptr++;
      DEBUG(
        "  [%02u] 0x%04X RK:%u CO:%u SU:%u ER:%u IS:%u WK:%2u WC:%2u\n",
        i,
        x,
        x & 7,
        (x & BIT(4)) != 0,
        (x & BIT(5)) != 0,
        (x & BIT(6)) != 0,
        (x & BIT(7)) != 0,
        (x >> 8) & 7,
        x >> 12
      );
    }
  }
  {
    DEBUG("Use & Update\n");
    for (uint8 i = 0; i <= 7; i++) {
      DEBUG("  [%02u] 0x%02X 0x%02X\n", i, config[52 + i + i], config[53 + i + i]);
    }
  }
  {
    DEBUG("Last Key Use\n");
    for (uint8 i = 0; i <= 15; i++) {
      DEBUG("  [%02u] 0x%02X\n", i, config[68 + i]);
    }
  }
  DEBUG("User Extra      0x%02X\n", config[84]);
  DEBUG("Selector        0x%02X\n", config[85]);
  DEBUG("Lock Data       0x%02X Data & OTP zone is %s\n", config[86], config[86] == 0x55 ? "WO" : "RW");
  DEBUG("Lock Config     0x%02X Configuration zone is %s\n", config[87], config[87] == 0x55 ? "RW" : "RO");

  #endif

  //result = sha204e_lock_config_zone(SHA204_HOST_ADDRESS);
  result = sha204e_lock_data_zone(SHA204_HOST_ADDRESS);

  return true;
}

/*
Byte # \t          Name    \t\t\t  Value \t\t\t  Description\n
0 - 3 \t   SN[0-3]           \t\t 012376ab   \t part of the serial number\n
4 - 7 \t   RevNum            \t\t 00040500   \t device revision (= 4)\n
8 - 12\t   SN[4-8]           \t\t 0c8fb7bdee \t part of the serial number\n
13    \t\t Reserved        \t\t\t 55       \t\t set by Atmel (55: First 16 bytes are unlocked / special case.)\n
14    \t\t I2C_Enable        \t\t 01       \t\t SWI / I2C (1: I2C)\n
15    \t\t Reserved        \t\t\t 00       \t\t set by Atmel\n
16    \t\t I2C_Address       \t\t c8       \t\t default I2C address\n
17    \t\t RFU         \t\t\t\t\t 00       \t\t reserved for future use; must be 0\n
18    \t\t OTPmode         \t\t\t 55       \t\t 55: consumption mode, not supported at this time\n
19    \t\t SelectorMode      \t\t 00       \t\t 00: Selector can always be written with UpdateExtra command.\n
20    \t\t slot  0, read   \t\t\t 8f       \t\t 8: Secret. f: Does not matter.\n
21    \t\t slot  0, write  \t\t\t 80       \t\t 8: Never write. 0: Does not matter.\n
22    \t\t slot  1, read   \t\t\t 80       \t\t 8: Secret. 0: CheckMac copy\n
23    \t\t slot  1, write  \t\t\t a1       \t\t a: MAC required (roll). 1: key id\n
24    \t\t slot  2, read   \t\t\t 82       \t\t 8: Secret. 2: Does not matter.\n
25    \t\t slot  2, write  \t\t\t e0       \t\t e: MAC required (roll) and write encrypted. 0: key id\n
26    \t\t slot  3, read   \t\t\t a3       \t\t a: Single use. 3: Does not matter.\n
27    \t\t slot  3, write  \t\t\t 60       \t\t 6: Encrypt, MAC not required (roll). 0: Does not matter.\n
28    \t\t slot  4, read   \t\t\t 94       \t\t 9: CheckOnly. 4: Does not matter.\n
29    \t\t slot  4, write  \t\t\t 40       \t\t 4: Encrypt. 0: key id\n
30    \t\t slot  5, read   \t\t\t a0       \t\t a: Single use. 0: key id\n
31    \t\t slot  5, write  \t\t\t 85       \t\t 8: Never write. 5: Does not matter.\n
32    \t\t slot  6, read   \t\t\t 86       \t\t 8: Secret. 6: Does not matter.\n
33    \t\t slot  6, write  \t\t\t 40       \t\t 4: Encrypt. 0: key id\n
34    \t\t slot  7, read   \t\t\t 87       \t\t 8: Secret. 7: Does not matter.\n
35    \t\t slot  7, write  \t\t\t 07       \t\t 0: Write. 7: Does not matter.\n
36    \t\t slot  8, read   \t\t\t 0f       \t\t 0: Read. f: Does not matter.\n
37    \t\t slot  8, write  \t\t\t 00       \t\t 0: Write. 0: Does not matter.\n
38    \t\t slot  9, read   \t\t\t 89       \t\t 8: Secret. 9: Does not matter.\n
39    \t\t slot  9, write  \t\t\t f2       \t\t f: Encrypt, MAC required (create). 2: key id\n
40    \t\t slot 10, read   \t\t\t 8a       \t\t 8: Secret. a: Does not matter.\n
41    \t\t slot 10, write  \t\t\t 7a       \t\t 7: Encrypt, MAC not required (create). a: key id\n
42    \t\t slot 11, read   \t\t\t 0b       \t\t 0: Read. b: Does not matter.\n
43    \t\t slot 11, write  \t\t\t 8b       \t\t 8: Never Write. b: Does not matter.\n
44    \t\t slot 12, read   \t\t\t 0c       \t\t 0: Read. c: Does not matter.\n
45    \t\t slot 12, write  \t\t\t 4c       \t\t 4: Encrypt, not allowed as target. c: key id\n
46    \t\t slot 13, read   \t\t\t dd       \t\t d: CheckOnly. d: key id\n
47    \t\t slot 13, write  \t\t\t 4d       \t\t 4: Encrypt, not allowed as target. d: key id\n
48    \t\t slot 14, read   \t\t\t c2       \t\t c: CheckOnly. 2: key id\n
49    \t\t slot 14, write  \t\t\t 42       \t\t 4: Encrypt. 2: key id\n
50    \t\t slot 15, read   \t\t\t af       \t\t a: Single use. f: Does not matter.\n
51    \t\t slot 15, write  \t\t\t 8f       \t\t 8: Never write. f: Does not matter.\n
52    \t\t UseFlag 0     \t\t\t\t ff       \t\t 8 uses\n
53    \t\t UpdateCount 0     \t\t 00       \t\t count = 0\n
54    \t\t UseFlag 1     \t\t\t\t ff       \t\t 8 uses\n
55    \t\t UpdateCount 1     \t\t 00       \t\t count = 0\n
56    \t\t UseFlag 2     \t\t\t\t ff       \t\t 8 uses\n
57    \t\t UpdateCount 2     \t\t 00       \t\t count = 0\n
58    \t\t UseFlag 3     \t\t\t\t 1f       \t\t 5 uses\n
59    \t\t UpdateCount 3     \t\t 00       \t\t count = 0\n
60    \t\t UseFlag 4     \t\t\t\t ff       \t\t 8 uses\n
61    \t\t UpdateCount 4     \t\t 00       \t\t count = 0\n
62    \t\t UseFlag 5     \t\t\t\t 1f       \t\t 5 uses\n
63    \t\t UpdateCount 5     \t\t 00       \t\t count = 0\n
64    \t\t UseFlag 6     \t\t\t\t ff       \t\t 8 uses\n
65    \t\t UpdateCount 6     \t\t 00       \t\t count = 0\n
66    \t\t UseFlag 7     \t\t\t\t ff       \t\t 8 uses\n
67    \t\t UpdateCount 7     \t\t 00       \t\t count = 0\n
68 - 83 \t LastKeyUse      \t\t\t 1fffffffffffffffffffffffffffffff\n
84    \t\t UserExtra\n
85    \t\t Selector    \t\t\t\t\t 00       \t\t Pause command with chip id 0 leaves this device active.\n
86    \t\t LockValue     \t\t\t\t 55       \t\t OTP and Data zones are not locked.\n
87    \t\t LockConfig    \t\t\t\t 55       \t\t Configuration zone is not locked.\n
*/

/*---------------------------------------------------------------------------*/
bool ATSHA204_Random(uint8_t *data)
{
}
/*---------------------------------------------------------------------------*/
bool ATSHA204_ReadData(uint8_t *data)
{
  sha204p_init();
  sha204p_set_device_id(SHA204_CLIENT_ADDRESS);

  uint8_t response[READ_32_RSP_SIZE];
  uint8_t ret_code = sha204c_wakeup(response);
  if (ret_code != SHA204_SUCCESS)
    return ret_code;

  /*
  uint8 tx_buffer[20];
  uint8 rx_buffer[50];
  ret_code = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_DATA | READ_ZONE_MODE_32_BYTES, 0);
  sha204p_sleep();
  int i = 0;
  */
}
/*---------------------------------------------------------------------------*/
