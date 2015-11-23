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
#include "HALLIS.h"
#include "HAL.h"
/*---------------------------------------------------------------------------*/
#undef DEBUG
#ifdef STARTUP_FROM_RESET
  #define DEBUG(...)
#else
  #if 1
    extern int debug_printf(const char *format,  ...);
    #define DEBUG debug_printf
  #else
    #define DEBUG(...)
  #endif
#endif
/*---------------------------------------------------------------------------*/
#ifndef INITIALIZE_STACK
  #error INITIALIZE_STACK
#endif
/*---------------------------------------------------------------------------
  LIS
  ---------------------------------------------------------------------------*/
#define INT32MAX 0x7FFFFFF
/*---------------------------------------------------------------------------*/
/* LIS register defines */
#define A_WHO_AM_I        0x0F
#define A_OFFSET_X        0x16
#define A_OFFSET_Y        0x17
#define A_OFFSET_Z        0x18
#define A_GAIN_X          0x19
#define A_GAIN_Y          0x1A
#define A_GAIN_Z          0x1B
#define A_CTRL_REG1       0x20
#define A_CTRL_REG2       0x21
#define A_CTRL_REG3       0x22
#define A_HP_FILTER_RESET 0x23
#define A_STATUS_REG      0x27
#define A_OUTX_L          0x28
#define A_OUTX_H          0x29
#define A_OUTY_L          0x2A
#define A_OUTY_H          0x2B
#define A_OUTZ_L          0x2C
#define A_OUTZ_H          0x2D
#define A_FF_WU_CFG       0x30
#define A_FF_WU_SRC       0x31
#define A_FF_WU_THS_L     0x34
#define A_FF_WU_THS_H     0x35
#define A_FF_WU_DURATION  0x36
#define A_DD_CFG          0x38
#define A_DD_SRC          0x39
#define A_DD_THSI_L       0x3C
#define A_DD_THSI_H       0x3D
#define A_DD_THSE_L       0x3E
#define A_DD_THSE_H       0x3F
/*---------------------------------------------------------------------------*/
/*
  Current accel configuration

   Pn,  640Hz,  no self test,  all axes on

    __7_____6_____5_____4_____3_____2_____1_____0__
   | PD1 | PD0 | DF1 | DF0 | ST  | Zen | Yen | Xen |
   |  1     1  |  1     0  |  0  |  1  |  1  |  1  |
     \_______/   \_______/    |     |     |     |X-axis enable
         |           |        |     |     |Y-axis enable
         |           |        |     |Z-axis enable
         |           |        |Self Test Enable
         |           |Decimate by 32 = 640Hz
         |Device on
*/
#define A_CTRL_REG1_PRESET 0xE7
/*---------------------------------------------------------------------------*/
/*
  +-2g,  update on read,  little endian,  no boot,  DRDY active,  4 wire interface,  12bit

    __7_____6_____5_____4_____3_____2_____1_____0__
   | FS  | BDU | BLE | BOOT| IEN | DRDY| SIM | DAS |
   |  0  |  1  |  0  |  0  |  0  |  1  |  0  |  0  |
      |     |     |     |     |     |     |     |12 bit right justified
      |     |     |     |     |     |     |4-wire interface
      |     |     |     |     |     |Enable Data-Ready generation
      |     |     |     |     |Data ready on RDY pad
      |     |     |     |No reboot memory content
      |     |     |Little endian
      |     |Output registers not updated between MSB and LSB reading
      |+-2g
*/
#define A_CTRL_REG2_PRESET 0x44
/*---------------------------------------------------------------------------*/
/*
  No hp-filter

    __7_____6_____5_____4_____3_____2_____1_____0__
   | ECK | HPDD| HPFF| FDS |     |     | CFS1| CFS0|
   |  0  |  0  |  0  |  0  |  0  |  0  |  0     0  |
      |     |     |     |                \_______/
      |     |     |     |                    |High-pass filter Cut-off Frequency = 512
      |     |     |     |Internal filter bypassed
      |     |     |Filter bypassed (Free-Fall and Wake-Up)
      |     |Filter bypassed (Direction Detection)
      |Clock from internal oscillator
*/
#define A_CTRL_REG3_PRESET 0x00
/*---------------------------------------------------------------------------*/
typedef struct Accel {
  bool open;
  uint16_t timeOut;       /* Timout counter */
  uint32_t sampleCount;   /* Number of caputured samples */
  uint8_t state;          /* The LIS status register */
  XYZ16 actual;           /* Actual captured sample */
  XYZ32 sum;              /* Sum of the captured samples */
  uint8_t sumCount;       /* Number of sums */
  uint32_t sumNoise;      /* Accumulated noise from xyz */
  XYZ16 avr;              /* Avarage of 10ms */
  uint16_t noise;         /* Noise over 10ms */
} Accel;
volatile Accel accel;
/*---------------------------------------------------------------------------*/
/*!
 * \brief Function to send 8bit value to the LIS and read a 8bit value
 * from the LIS
 * \param send The value to send
 * \return The read value
 */
static uint8_t Write8Read8(uint8_t send)
{
  return HAL_LISWrite16Read8(send, 0xFF);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Function to send 16bit value to the LIS and read a 8bit value
 * from the LIS
 * \param send1 The high byte value to send
 * \param send2 The low byte value to send
 * \return The read value
 */
static uint8_t Write16Read8(uint8_t send1, uint8_t send2)
{
  return HAL_LISWrite16Read8(send1, send2);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Function to write a 8bit data to a address in the LIS
 * \param addr The address for the data
 * \param data The data to write
 */
static uint8_t Write8(uint8_t addr, uint8_t data)
{
  return Write16Read8(addr, data);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Function to read 8bit data from a address in the LIS
 * \param addr The address of the data
 * \return The data read
 */
static uint8_t Read8(uint8_t addr)
{
  return Write8Read8(addr | 0x80);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 * \note The function is called with a rate of 1000Hz. The LIS samples with
 * a rate of 640Hz. So every sample is catched.
 */
void HALLIS_Server1MS(void)
{
  if (!accel.open) return;

  /* d = | a - b |,  limit b to max int16 */
  uint16_t DivAbs16(int16_t a, int16_t b)
  {
    int32_t d = (int32_t) a - (int32_t) b;
    if (d < 0) d = -d;
    return d > (int32_t) INT32MAX ? INT32MAX : d;
  }

  uint8_t state = Read8(A_STATUS_REG);
  if ((state & 0x07) == 0x07) {
    accel.state = state;
    accel.timeOut = 0;

    /* Get XYZ values from sensor and clear the data ready flags (this three reads do this).
       Note xy is swapped,  y is negative. Every channel has 12 bits with sign extension to 16bit */
    XYZ16 actual;
    actual.x = + (int16_t) (((uint16_t) Read8(A_OUTY_H) << 8) | Read8(A_OUTY_L));
    actual.y = + (int16_t) (((uint16_t) Read8(A_OUTX_H) << 8) | Read8(A_OUTX_L));
    actual.z = - (int16_t) (((uint16_t) Read8(A_OUTZ_H) << 8) | Read8(A_OUTZ_L));

    /* Accumulate the withe noise of all three channels.
       The maximum nois per cahnnel could be 32767. So maximum 128 accumulated samples on three channels:
       32767 * 128 * 3 = 12582528 = 0x00BFFE80. accel.sumNoise can't overflow. */
    accel.sumNoise += DivAbs16(accel.actual.x, actual.x) + DivAbs16(accel.actual.y, actual.y) + DivAbs16(accel.actual.z, actual.z);

    accel.actual = actual;
    accel.sampleCount++;

    /* Accumulate the three axis. Value raneg ist 12bit (-2048..+2047). So maximum 128 accumulated samples:
       2048 * 128 = 262144 = 0x00040000. accel.sum.? can't overflow */
    accel.sum.x += accel.actual.x;
    accel.sum.y += accel.actual.y;
    accel.sum.z += accel.actual.z;
    accel.sumCount++;

    /* If the data is not polled: After 128 samples crunch (divide) the buffers by 16.
       This is for savety. */
    if (accel.sumCount > 128) {
      accel.sum.x /= 16;
      accel.sum.y /= 16;
      accel.sum.z /= 16;
      accel.sumCount >>= 4;
      accel.sumNoise >>= 4;
    }
  }

  /* Downsample by 10. Se effective sample rate ist 100Hz */
  static uint8_t subSample = 0;
  if (subSample++ >= 10) {
    subSample = 0;
    accel.avr.x = (int16_t) (accel.sum.x / accel.sumCount);
    accel.avr.y = (int16_t) (accel.sum.y / accel.sumCount);
    accel.avr.z = (int16_t) (accel.sum.z / accel.sumCount);
    accel.sum.x = accel.sum.y = accel.sum.z = accel.sumCount = 0;
    accel.noise = (uint16_t) accel.sumNoise;
    accel.sumNoise = 0;
  }
}
/*---------------------------------------------------------------------------*/
int16_t HALLIS_GetXYZ(XYZ16 *xyz)
{
  if (!accel.open) 
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  GlobalInterruptsDisable();
  *xyz = accel.avr;
  GlobalInterruptsEnable();
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int32_t HALLIS_GetNoise(void)
{
  if (!accel.open) 
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  return accel.noise;
}
/*---------------------------------------------------------------------------*/
int16_t HALLIS_Open(void)
{
  int8_t result = HAL_LISOpen();
  if (result < 0) 
    return result;
  HAL_LISReset();
  HAL_DelayMS(100);
  for (uint8_t i = 0;i < 128;i++) 
    if (Read8(A_WHO_AM_I) == 0x3A) 
      break;
  if (Read8(A_WHO_AM_I) != 0x3A) 
    return RESULT_HAL_ERROR_DEVICE_NOT_FOUND;

  DEBUG("LIS3LV02 found\n");
  /* Init LIS */
  (void) Write8(A_CTRL_REG1, A_CTRL_REG1_PRESET);
  (void) Write8(A_CTRL_REG2, A_CTRL_REG2_PRESET);
  (void) Write8(A_CTRL_REG3, A_CTRL_REG3_PRESET);
  DEBUG("  Offset %d %d %d\n", Read8(A_OFFSET_Z), Read8(A_OFFSET_Y), Read8(A_OFFSET_X));
  DEBUG("  Gain %d %d %d\n", Read8(A_GAIN_Z), Read8(A_GAIN_Y), Read8(A_GAIN_X));
  DEBUG("  Ctrl %02X %02X %02X\n", Read8(A_CTRL_REG1), Read8(A_CTRL_REG2), Read8(A_CTRL_REG3));
  DEBUG("  Status %02X\n", Read8(A_STATUS_REG));

  accel.open = true;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HALLIS_Close(void)
{
  accel.open = false;
  HAL_LISClose();
}
/*---------------------------------------------------------------------------*/
