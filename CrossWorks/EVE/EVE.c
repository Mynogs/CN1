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
#include "EVE.h"
#include "../HAL/Hal.h"
#include "FT800.h"
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
#define RED          0xFF0000UL
#define GREEN        0x00FF00UL
#define BLUE         0x0000FFUL
#define YELLOW       0xFFFF00UL
#define WHITE        0xFFFFFFUL
#define BLACK        0x000000UL
/*---------------------------------------------------------------------------*/
const LCD _LCD_QVGA  = {320,240,408,70,0,10,263,13,0,2,8,2,0};
const LCD _LCD_WQVGA = {480,272,548,43,0,41,292,12,0,10,5,0,1};
/*---------------------------------------------------------------------------*/
typedef struct EVE {
  uint16_t commandRead;
  uint16_t commandWrite;
} EVE;
EVE eve;
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void CommandWrite(uint8_t command)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer(command);
  HAL_EVETransfer(0);
  HAL_EVETransfer(0);
  HAL_EVETransferEnd();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void MemoryWrite8(uint32_t address, uint8_t data)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_WRITE);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(data);
  HAL_EVETransferEnd();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void MemoryWrite16(uint32_t address, uint16_t data)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_WRITE);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(data);
  HAL_EVETransfer(data >> 8);
  HAL_EVETransferEnd();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void MemoryWrite32(uint32_t address, uint32_t data)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_WRITE);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(data);
  HAL_EVETransfer(data >> 8);
  HAL_EVETransfer(data >> 16);
  HAL_EVETransfer(data >> 24);
  HAL_EVETransferEnd();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void MemoryWriteN(uint32_t address, uint8_t *data, uint16_t size)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_WRITE);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  while (size--)
    HAL_EVETransfer(*data++);
  HAL_EVETransferEnd();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static uint8_t MemoryRead8(uint32_t address)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_READ);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(0);
  uint8_t data = HAL_EVETransfer(0);
  HAL_EVETransferEnd();
  return data;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static uint16_t MemoryRead16(uint32_t address)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_READ);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(0);
  uint16_t data = HAL_EVETransfer(0);
  data |= (uint16_t) HAL_EVETransfer(0) << 8;
  HAL_EVETransferEnd();
  return data;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static uint32_t MemoryRead32(uint32_t address)
{
  HAL_EVETransferBegin();
  HAL_EVETransfer((address >> 16) | MEM_READ);
  HAL_EVETransfer(address >> 8);
  HAL_EVETransfer(address);
  HAL_EVETransfer(0);
  uint32_t data = HAL_EVETransfer(0);
  data |= (uint16_t) HAL_EVETransfer(0) << 8;
  data |= (uint32_t) HAL_EVETransfer(0) << 16;
  data |= (uint32_t) HAL_EVETransfer(0) << 24;
  HAL_EVETransferEnd();
  return data;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
uint16_t incCMDOffset(uint16_t offset, uint8_t size)
{
  return (offset + size) % 4096;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
int16_t EVE_Open(uint16_t channel)
{
  memset(&eve, 0, sizeof(eve));
  int16_t result = HAL_EVEOpen(channel);
  if (result < 0) return result;
  CommandWrite(FT800_ACTIVE);  // Start FT800
  HAL_DelayMS(5);
  CommandWrite(FT800_CORERST); // Reset FT800
  HAL_DelayMS(5);
  CommandWrite(FT800_ACTIVE);  // Start FT800
  HAL_DelayMS(5);
  CommandWrite(FT800_CLKEXT);  // Set FT800 for external clock
  HAL_DelayMS(5);
  CommandWrite(FT800_CLK48M);  // Set FT800 for 48MHz PLL
  HAL_DelayMS(5);
  HAL_EVESetSpeed(30E6);       // Now FT800 can accept commands at up to 30MHz clock on SPI bus

  {
    uint8_t id;
    for (uint8_t i = 0; i < 10; i++) {
      id = MemoryRead8(REG_ID);
      if (id == 0x7C) break;
      HAL_DelayMS(5);
    }
    if (id != 0x7C)
      return RESULT_HAL_ERROR_DEVICE_NOT_WORK;
  }

  MemoryWrite8(REG_PCLK, ZERO);      // Set PCLK to zero - don't clock the LCD until later
  MemoryWrite8(REG_PWM_DUTY, ZERO);  // Turn off backlight

  // Initialize Display
  const LCD *lcd = &_LCD_WQVGA;
  MemoryWrite16(REG_HSIZE,   lcd->width);
  MemoryWrite16(REG_HCYCLE,  lcd->hCycle);
  MemoryWrite16(REG_HOFFSET, lcd->hOffset);
  MemoryWrite16(REG_HSYNC0,  lcd->hSync0);
  MemoryWrite16(REG_HSYNC1,  lcd->hSync1);
  MemoryWrite16(REG_VSIZE,   lcd->height);
  MemoryWrite16(REG_VCYCLE,  lcd->vCycle);
  MemoryWrite16(REG_VOFFSET, lcd->vOffset);
  MemoryWrite16(REG_VSYNC0,  lcd->vSync0);
  MemoryWrite16(REG_VSYNC1,  lcd->vSync1);
  MemoryWrite8(REG_SWIZZLE,  lcd->swizzle);
  MemoryWrite8(REG_PCLK_POL, lcd->pclkPol);

  // Configure Touch and Audio - not used in this example, so disable both
  MemoryWrite8(REG_TOUCH_MODE, ZERO);        // Disable touch
  MemoryWrite16(REG_TOUCH_RZTHRESH, ZERO);   // Eliminate any false touches

  MemoryWrite8(REG_VOL_PB, ZERO);            // turn recorded audio volume down
  MemoryWrite8(REG_VOL_SOUND, ZERO);         // turn synthesizer volume down
  MemoryWrite16(REG_SOUND, 0x6000);          // set synthesizer to mute

  // Write Initial Display List & Enable Display
  MemoryWrite32(RAM_DL, DL_CLEAR_RGB);                               // Clear Color RGB   00000010 RRRRRRRR GGGGGGGG BBBBBBBB  (R/G/B = Colour values) default zero / black
  MemoryWrite32(RAM_DL + 4, DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG); // Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear)
  MemoryWrite32(RAM_DL + 8, DL_DISPLAY);                             // DISPLAY command 00000000 00000000 00000000 00000000 (end of display list)
  MemoryWrite32(RAM_DL + 12, CMD_SWAP); // ??????
  //MemoryWrite32(REG_DLSWAP, DLSWAP_FRAME);                         // 00000000 00000000 00000000 000000SS  (SS bits define when render occurs)
                                                                     // Nothing is being displayed yet... the pixel clock is still 0x00

  uint8_t ft800Gpio = MemoryRead8(REG_GPIO); // Read the FT800 GPIO register for a read/modify/write operation
  ft800Gpio = ft800Gpio | 0x80;              // set bit 7 of FT800 GPIO register (DISP) - others are inputs
  MemoryWrite8(REG_GPIO, ft800Gpio);         // Enable the DISP signal to the LCD panel
  MemoryWrite8(REG_PCLK, lcd->pClk);         // Now start clocking data to the LCD panel
  MemoryWrite8(REG_PWM_DUTY, 128);           // Turn on backlight

  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void EVE_Close(void)
{
  HAL_EVEClose();
}
/*---------------------------------------------------------------------------*/
int16_t EVE_CommandBegin(void)
{
  //DEBUG("--\n");
  do {
    eve.commandRead = MemoryRead16(REG_CMD_READ);         // Read the graphics processor read pointer
    if (eve.commandRead == 0x0FFF) {
      //return RESULT_EVE_ERROR_COPROCESSOR;
      DEBUG("cotz protz error\n");
      MemoryWrite16(REG_CMD_READ, 0x0000);
      MemoryWrite16(REG_CMD_WRITE, 0x0000);
    }
    eve.commandWrite = MemoryRead16(REG_CMD_WRITE);       // Read the graphics processor write pointer
  } while (eve.commandRead != eve.commandWrite);          // Wait until the two registers match
  eve.commandWrite = (eve.commandWrite + 4) & 0x0FFC;     // % 4096 and longword address
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/

static bool once = true;

int16_t EVE_CommandAppend(uint32_t item)
{
  char Vis(char c)
  {
    return (c >= 32) && (c <= 126) ? c : '_';
  }

  MemoryWrite32(RAM_CMD + eve.commandWrite, item);
  //if (once) DEBUG("DL: %08X %c%c%c%c \n", item, Vis(item), Vis(item >> 8), Vis(item >> 16), Vis(item >> 24));
  eve.commandWrite = (eve.commandWrite + 4) % 4096;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t EVE_CommandAppendString(const char *s)
{
  uint8_t Get(void)
  {
    return *s ? *s++ : 0;
  }

  if (s && *s) {
    uint32_t item;
    while (*s) {
      item = Get();
      item |= (uint16_t) Get() << 8;
      item |= (uint32_t) Get() << 16;
      item |= (uint32_t) Get() << 24;
      EVE_CommandAppend(item);
    }
    if (item >> 24)
      EVE_CommandAppend(0);
  } else
    EVE_CommandAppend(0);
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t EVE_CommandEnd(void)
{
  EVE_CommandAppend(DL_DISPLAY);                  // Instruct the graphics processor to show the list
  EVE_CommandAppend(CMD_SWAP);                    // Make this list active
  MemoryWrite16(REG_CMD_WRITE, eve.commandWrite); // Update the ring buffer pointer so the graphics processor starts executing

  once = false;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t EVE_WriteMain(uint32_t address, uint8_t *data, uint16_t size)
{
  if (address + size > 256 * 1024)
    return -1;
  MemoryWriteN(RAM_G + address, data, size);
  return 0;
}
/*---------------------------------------------------------------------------*/
int16_t EVE_WritePalette(uint32_t address, uint8_t *data, uint16_t size)
{
  if (address + size > 1024)
    return -1;
  MemoryWriteN(RAM_PAL + address, data, size);
  return 0;
}
/*---------------------------------------------------------------------------*/




/*---------------------------------------------------------------------------*/
void EVE_DisplayListTest(void)
{
  uint8_t toggle;
  int16_t x = (196 * 16);
  int16_t y = (136 * 16);
  int16_t sy = 0;
  uint16_t color;
  for (;;) {
    uint32_t color = toggle++ & 0x80 ? RED :  YELLOW;

    EVE_CommandBegin();
    EVE_CommandAppend(CMD_DLSTART);
    EVE_CommandAppend(DL_CLEAR_RGB | BLACK);
    EVE_CommandAppend(DL_CLEAR | CLR_COL | CLR_STN | CLR_TAG);
    EVE_CommandAppend(DL_COLOR_RGB | color);
    EVE_CommandAppend(DL_POINT_SIZE | 0x100);
    EVE_CommandAppend(DL_BEGIN | FTPOINTS);
    EVE_CommandAppend(DL_VERTEX2F | (x << 15) | (((272 - 16) * 16) - y));
    EVE_CommandAppend(DL_END);
    EVE_CommandEnd();
    HAL_DelayMS(10);
    /**/
    y = y + sy;
    if (y < 0) {
      y = 0;
      sy = -sy;
    }
    sy -= 4;
    /**/

  }
/*
DL: 02000000
DL: 26000007
DL: 04FFFF00
DL: 0D000100
DL: 1F000002
DL: 46200780
DL: 21000000
DL: 00000000
DL: FFFFFF01
*/

}
/*---------------------------------------------------------------------------*/






