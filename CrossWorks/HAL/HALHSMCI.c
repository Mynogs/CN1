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
#define USE_DMA 1
/*---------------------------------------------------------------------------*/
#include "HALHSMCI.h"
#include "HAL.h"
/*---------------------------------------------------------------------------*/
#undef DEBUG
#undef DEBUG1
#undef DEBUG2
#ifdef STARTUP_FROM_RESET
  #define DEBUG1(...)
  #define DEBUG2(...)
#else
  #if 1
    extern int debug_printf(const char *format, ...);
    #define DEBUG1 debug_printf
  #else
    #define DEBUG1(...)
  #endif
  #if 0
    extern int debug_printf(const char *format, ...);
    #define DEBUG2 debug_printf
  #else
    #define DEBUG2(...)
  #endif
#endif
/*---------------------------------------------------------------------------*/
// Commands
#define ACMD_SET_BUS_WIDTH        6
#define ACMD_OP_COND             41

#define CMD_GO_IDLE_STATE         0
#define CMD_ALL_SEND_CID          2
#define CMD_SET_RELATIVE_ADDR     3
#define CMD_SELECT_CARD           7
#define CMD_SEND_IF_COND          8
#define CMD_SEND_CSD              9
#define CMD_SEND_CID             10
#define CMD_STOP_TRANSMISSION    12
#define CMD_SEND_STATUS          13
#define CMD_READ_SINGLE_BLOCK    17
#define CMD_READ_MULTIPLE_BLOCK  18
#define CMD_WRITE_BLOCK          24
#define CMD_WRITE_MULTIPLE_BLOCK 25
#define CMD_APP_CMD              55
/*---------------------------------------------------------------------------*/
#define BLOCK_SIZE      512
#define MAX_DMA_BLOCKS  127
/*---------------------------------------------------------------------------*/
#define MCI_ERRORS_MASK \
  (HSMCI_SR_RINDE | HSMCI_SR_RDIRE | HSMCI_SR_RENDE | HSMCI_SR_RTOE | HSMCI_SR_DCRCE | HSMCI_SR_DTOE | HSMCI_SR_OVRE | HSMCI_SR_UNRE)
/*---------------------------------------------------------------------------*/
struct {
  uint32_t relativeCardAddress; // Relative Card Address (RCA)
  uint32_t numberOfBlocks;      // Number of blocks on card
  uint32_t actualBlockSize;     // Actual block size
  uint32_t sdhcFlag;
} hsmci;
/*---------------------------------------------------------------------------*/
/*!
 * \brief Send a command
 */
static int16_t WaitForStatusReady(void)
{
  uint32_t timeout = 30000, status;
  do {
    if (!timeout--) {
      DEBUG2("Timeout!\n");
      return RESULT_ERROR_TIMEOUT;
    }
    status = HSMCI->HSMCI_SR;
  } while (!(status & HSMCI_SR_CMDRDY));
  if (status & MCI_ERRORS_MASK) {
    DEBUG2("Error! Status is 0x%08X\n", status);
    return RESULT_ERROR;
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Send a command
 */
static int16_t SendCommand(uint32_t command, uint32_t argument)
{
  DEBUG2("SendCommand(0x%08X, 0x%08X)\n", command, argument);
  HSMCI->HSMCI_ARGR = argument;
  HSMCI->HSMCI_CMDR = command;
  return WaitForStatusReady();
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void GetResponse48(uint32_t *buffer)
{
  DEBUG2("Response48");
  while (!(HSMCI->HSMCI_SR & HSMCI_SR_CMDRDY)) {
    int i = 0;
  }
  for (uint8_t i = 0; i <= 1; i++) {
    *buffer = HSMCI->HSMCI_RSPR[i];
    DEBUG2(" %08X", *buffer++);
  }
  DEBUG2("\n");
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static void GetResponse136(uint32_t *buffer)
{
  DEBUG2("Response136");
  while (!(HSMCI->HSMCI_SR & HSMCI_SR_CMDRDY)) {
    int i = 0;
  }
  for (uint8_t i = 0; i <= 4; i++) {
    *buffer = HSMCI->HSMCI_RSPR[i];
    DEBUG2(" %08X", *buffer++);
  }
  DEBUG2("\n");
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD0 GO_IDLE_STATE
 * This is the software reset command and sets each card into Idle State
 * regardless of the current card state. Cards in Inactive State are not
 * affected by this command. After power on or CMD0, all cards CMD lines are
 * in input mode, waiting for start bit of the next command.
 * The cards are initialized with a default relative card address (RCA=0x0000)
 * and with a default driver stage register setting. (lowest speed,
 * highest driving current capability).
 */
int16_t SendCMD0(void)
{
  DEBUG2("CMD0\n");
  return SendCommand(CMD_GO_IDLE_STATE | HSMCI_CMDR_MAXLAT, 0);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD2 ALL_SEND_CID
 * Asks any card to send the CID numbers on the CMD line (any card that is
 * connected to the host will respond)
 */
int16_t SendCMD2(void)
{
  DEBUG2("CMD2");
  // Card IDentification register CMD2
  int16_t result = SendCommand(CMD_ALL_SEND_CID | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_136_BIT, 0);
  uint32_t response[5];
  GetResponse136(response);
  return result;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD3 SEND_RELATIVE_ADDR
 * Ask the card to publish a new relative address (RCA).
 * \return >= 0: RelativeCardAddress (RCA)
 */
int32_t SendCMD3(void)
{
  DEBUG2("CMD3\n");
  int16_t result = SendCommand(CMD_SET_RELATIVE_ADDR | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT, 0);
  if (result < 0) return result;
  uint32_t response[2];
  GetResponse48(response); // Response R6
  return (response[0] >> 16) & 0xFFFF; // Extract relative card address
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD8 SEND_IF_COND
 * Send Interface Condition Command is defined to initialize SD Memory Cards
 * compliant to the Physical Specification Version 2.00. CMD8 is valid when
 * the card is in Idle state. This command has two functions:
 * Voltage check
 *   Checks whether the card can operate on the host supply voltage.
 * Enabling expansion of existing command and response
 *   Reviving CMD8 enables to expand new functionality to some existing
 *   commands by redefining previously reserved bits. ACMD41 is expanded to
 *   support initialization of High Capacity SD Memory Cards.
 */
int16_t SendCMD8(void)
{
  DEBUG2("CMD8\n");
  // CMD8 with 0b10101010 arg and VHS = 1
  uint8_t checkPattern = 0xAA;
  uint16_t result = SendCommand(CMD_SEND_IF_COND | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT, 0x00000100 | checkPattern);
  if (result < 0) return result;
  uint32_t response[2];
  GetResponse48(response);
  return (response[0] & 0xFF) == checkPattern ? RESULT_OK : RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD9 SEND_CSD
 */
int16_t SendCMD9(void)
{
  DEBUG2("CMD9\n");



}
/*---------------------------------------------------------------------------*/
/*!
 * \brief CMD55 APP_CMD
 * Indicates to the card that the next command is an application specific
 * command rather than a standard command
 */
int16_t SendCMD55(void)
{
  DEBUG2("CMD55\n");
  int16_t result = SendCommand(CMD_APP_CMD | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT,0);
  if (result < 0) return result;
  uint32_t response[2];
  GetResponse48(response);
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief ACMD41 SD_SEND_OP_COND
 * Sends host capacity support information (HCS) and asks the accessed card to
 * send its operating condition register (OCR) content in the response on the
 * CMD line.
 * HCS is effective when card receives SEND_IF_COND command.
 * Reserved bit shall be set to 0. CCS bit is assigned to OCR[30].
 * \return bit1: ready flag, bit9: ?
 */
int16_t SendACMD41(void)
{
  int16_t result = SendCMD55(); // Switch to application specific command
  if (result < 0) return result;
  // Bit 30 is HCS bit and FF8 is voltage mask (see page 55 of SDHC simplified spec)
  result = SendCommand(ACMD_OP_COND | HSMCI_CMDR_RSPTYP_48_BIT, 0x00FF8000 | BIT(30));
  if (result < 0) return result;
  uint32_t response[2];
  GetResponse48(response);
  return response[0] >> 30;
  //return (response[0] & BIT(31)) != 0;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Waits for ready status and checks CSS bit OCR contents
 * If CSS is 1 after status changed to "ready", it sets global sdhcFlag to 1
 * otherwise sdhcFlag is set to 0.
 */
static int16_t WaitForReady(void)
{
  for (uint16_t i = 0; i < 100; i++) {
    HAL_DelayMS(10);
    int16_t result = SendACMD41();
    if (result < 0) return result;
    if (result & BIT(1)) {
      hsmci.sdhcFlag = result & BIT(0);
      return RESULT_OK;
    }
  }
  DEBUG2("WaitForReady: TIMEOUT!\n");
  return RESULT_ERROR_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 * \note This init func is not 100% compatilbe with SDHC specification!
 * It is simple and dumb: SDHC cards are detected by checking CSS bit in busy wait func.
 * CMD8 is issued just for compatibility and it doesnt change anything in the program.
 * The init func supports neither SDIO nor MMC cards. See Figure 7-2 in SDHC Simplified spec
 * for more details about standard init procedure.
 */
static int16_t CardInit(void)
{
  uint32_t response[5];

  int32_t result = SendCommand(HSMCI_CMDR_SPCMD_INIT, 0); // Initialization command (74 clock cycles for initialization sequence)
  if (result < 0)
    return result;

  result = SendCMD0();
  if (result < 0)
    return result;

  result = SendCMD8();
  if (result < 0)
    return result;

  result = WaitForReady();
  if (result < 0)
    return result;

  result = SendCMD2();
  if (result < 0)
    return result;

  result = SendCMD3();
  if (result < 0)
    return result;
  hsmci.relativeCardAddress = result;

  result = SendCMD9();
  if (result < 0)
    return result;


  //read CSD
  SendCommand(CMD_SEND_CSD | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_136_BIT, hsmci.relativeCardAddress << 16);
  GetResponse136(response);

  //compute block length and number of blockz
  //example of returned CSD stuff (SD 256MB)


  //005E0032 1F5983CF EDB6BF87 964000B5 005E0032
  //            ^ ^^^ ^
  //      bl.len| |||C_SIZE
  /*
    4GB SDHC:
    400E0032 5B590000 1DE77F80 0A4000D5 400E0032
    bl.len is fixed to 0x9 = 512B/block
  */

  //005E0032 1F5983CF EDB6BF87 964000B5 005E0032  256MB V1.0
  //      |/ |_/|
  //      |  |  | READ_BL_LEN 9:512, 10:1024 11:2048
  //      |  |
  //      |  | CCC 01x1 1011 0101
  //      |
  //      | TRAN_SPEED 0x32 or 0x5A
  //
  //400E0032 5B590000 1DE77F80 0A4000D5 400E0032  4GB V2.0
  //      |/ |_/|
  //      |  |  | READ_BL_LEN always 9:512
  //      |  |
  //      |  | CCC 01x1 1011 0101
  //      |
  //      | TRAN_SPEED 0x32 or 0x5A
  //

  DEBUG2("send CSD response:\n");

  // Compute number of blocks
  if (response[0] & 0x40000000) {
    // CSD V2.0
    uint32_t count1024 = ((response[1] << 16) | (response[2] >> 16)) & 0x3FFFFF;
    hsmci.numberOfBlocks = (count1024 + 1) * 1024;
    if ((response[1] & 0x000F0000) != 0x00090000) {
      DEBUG1("cardInit: wrong SDHC block size\n");
      return RESULT_ERROR;
    }
    hsmci.actualBlockSize = BLOCK_SIZE;
  } else {
    // CSD V1.0
    uint8_t *byte = (uint8_t*) response;
    uint32_t size = (uint32_t) (byte[5] & 0x03) << 10;
    size |= (uint16_t) byte[4] << 2;
    size |= byte[11] >> 6;
    uint16_t sizeMultiplier = 1 << (2 + (((byte[10] & 0x03) << 1) | (byte[9] >> 7)));
    hsmci.numberOfBlocks = sizeMultiplier * (size + 1);
    hsmci.actualBlockSize = 1 << ((response[1] >> 16) & 0x0F);
  }
  DEBUG2("actual block size:%u \n", hsmci.actualBlockSize);

  if ((response[0] & 0x000000FF) == 0x5A)
    // 50MHz
    HSMCI->HSMCI_MR = (HSMCI->HSMCI_MR & ~HSMCI_MR_CLKDIV_Msk) | HSMCI_MR_CLKDIV(1); // = 30MHz
  else
    // 25MHz
    HSMCI->HSMCI_MR = (HSMCI->HSMCI_MR & ~HSMCI_MR_CLKDIV_Msk) | HSMCI_MR_CLKDIV(2); // = 20MHz

  //select card (put it into the transfer state)
  SendCommand(CMD_SELECT_CARD | HSMCI_CMDR_RSPTYP_48_BIT | HSMCI_CMDR_MAXLAT, hsmci.relativeCardAddress << 16);
  GetResponse48(response);

  //set 4-bit bus width
  SendCommand(CMD_APP_CMD | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT, hsmci.relativeCardAddress << 16);
  GetResponse48(response);

  SendCommand(ACMD_SET_BUS_WIDTH | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT, 2);
  GetResponse48(response);

  /*SendCommand(16 | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, 512);
  GetResponse48(response);*/

  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static uint32_t ReadStatus(void)
{
  SendCommand(CMD_SEND_STATUS | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_RSPTYP_48_BIT, hsmci.relativeCardAddress << 16);
  uint32_t response[2];
  GetResponse48(response);
  return response[0];
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static int16_t ReadBlocksDMA(void *buffer, uint32_t lba, uint16_t blocksToRead)
{
  //enable PDC mode
  HSMCI->HSMCI_MR |= HSMCI_MR_PDCMODE;

  HSMCI->HSMCI_BLKR = (BLOCK_SIZE << 16 ) | 1;

  //configure the PDC channel
  HSMCI->HSMCI_RPR = (uint32_t) buffer;
  HSMCI->HSMCI_RCR = BLOCK_SIZE * blocksToRead;
  HSMCI->HSMCI_PTCR = HSMCI_PTCR_RXTEN;

  // Send CMD_READ_MULTIPLE_BLOCK (CMD18) command
  if (hsmci.sdhcFlag) {
    SendCommand(CMD_READ_MULTIPLE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRDIR | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRTYP_MULTIPLE | HSMCI_CMDR_RSPTYP_48_BIT, lba);
  } else {
    uint32_t byteAddress = lba * BLOCK_SIZE;
    SendCommand(CMD_READ_MULTIPLE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRDIR | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRTYP_MULTIPLE | HSMCI_CMDR_RSPTYP_48_BIT, byteAddress);
  }

  while ((HSMCI->HSMCI_SR & HSMCI_SR_ENDRX) == 0);

  SendCommand(CMD_STOP_TRANSMISSION | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_STOP_DATA | HSMCI_CMDR_RSPTYP_48_BIT, 0);

  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
//keep (blocksToWrite * BLOCK_SIZE) < 0x10000
static int16_t WriteBlocksDMA(const void *buffer, uint32_t lba, uint32_t blocksToWrite)
{
  //enable PDC mode
  HSMCI->HSMCI_MR |= HSMCI_MR_PDCMODE;

  //set block len in bytes
  HSMCI->HSMCI_BLKR = (BLOCK_SIZE << 16 ) | 1;

  HAL_DelayUS(250);

  if (hsmci.sdhcFlag) {
    SendCommand(CMD_WRITE_MULTIPLE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRTYP_MULTIPLE | HSMCI_CMDR_RSPTYP_48_BIT, lba);
  } else {
    uint32_t byteAddress = lba * BLOCK_SIZE;
    SendCommand(CMD_WRITE_MULTIPLE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRTYP_MULTIPLE | HSMCI_CMDR_RSPTYP_48_BIT, byteAddress);
  }

  HSMCI->HSMCI_TPR = (uint32_t)buffer;
  HSMCI->HSMCI_TCR = BLOCK_SIZE * blocksToWrite;

  //start DMA transfer
  HSMCI->HSMCI_PTCR = HSMCI_PTCR_TXTEN;

  //now poll BLKE bit
  while((HSMCI->HSMCI_SR & HSMCI_SR_BLKE) == 0);

  //DEBUG2("*CMD12");
  SendCommand(CMD_STOP_TRANSMISSION | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_STOP_DATA | HSMCI_CMDR_RSPTYP_48_BIT, 0);

  HSMCI->HSMCI_PTCR = HSMCI_PTCR_TXTDIS;

  //DEBUG2("waiting for NOTBUSY bit...");
  while ((HSMCI->HSMCI_SR & HSMCI_SR_NOTBUSY) == 0);

  while ((ReadStatus() & (1<<8))==0);

  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static int16_t ReadBlock(void *buffer, uint32_t lba)
{
  DEBUG2("ReadBlock(%u)\n", lba);
  //disable PDC mode (if enabled)
  HSMCI->HSMCI_MR &= ~HSMCI_MR_PDCMODE;

  HSMCI->HSMCI_BLKR = (BLOCK_SIZE << 16 ) | 1;

  if (hsmci.sdhcFlag) {
    SendCommand(CMD_READ_SINGLE_BLOCK | HSMCI_CMDR_MAXLAT  | HSMCI_CMDR_TRDIR | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_RSPTYP_48_BIT, lba);
  } else {
    uint32_t byteAddress = lba * BLOCK_SIZE;
    SendCommand(CMD_READ_SINGLE_BLOCK | HSMCI_CMDR_MAXLAT  | HSMCI_CMDR_TRDIR | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_RSPTYP_48_BIT, byteAddress);
  }

  uint32_t wordsToRead = BLOCK_SIZE / 4;
  uint8_t *pBuf = (uint8_t*) buffer;

  while (wordsToRead--) {
    uint32_t status;
    do {
      status = HSMCI->HSMCI_SR;
    } while (!(status & HSMCI_SR_RXRDY));
    uint32_t temp = HSMCI->HSMCI_RDR;
    for (uint8_t i = 0; i < 4; i++) {
      *pBuf++ = temp;
      temp = temp >> 8;
    }
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
static int16_t WriteBlock(const void *buffer, uint32_t lba)
{
  //disable PDC mode (if enabled)
  HSMCI->HSMCI_MR &= ~HSMCI_MR_PDCMODE;

  HSMCI->HSMCI_BLKR = (BLOCK_SIZE << 16 ) | 1;

  if (hsmci.sdhcFlag) {
    SendCommand(CMD_WRITE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_RSPTYP_48_BIT, lba);
  } else {
    uint32_t byteAddress = lba * BLOCK_SIZE;
    SendCommand(CMD_WRITE_BLOCK | HSMCI_CMDR_MAXLAT | HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_RSPTYP_48_BIT, byteAddress);
  }
  uint32_t wordsToWrite = BLOCK_SIZE / 4;
  uint8_t *pBuf = (uint8_t*)buffer;
  while (wordsToWrite--) {
    while (!(HSMCI->HSMCI_SR & HSMCI_SR_TXRDY));
    uint32_t temp;
    uint8_t *pTemp = (uint8_t*)&temp;
    for (uint8_t i = 0; i < 4; i++) {
      *pTemp = *pBuf;
      pBuf++;
      pTemp++;
    }
    HSMCI->HSMCI_TDR = temp;
  }
  //wait for NOTBUSY bit set
  while ((HSMCI->HSMCI_SR & HSMCI_SR_NOTBUSY) == 0);
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HALHSMCI_Init(void)
{
  //DEBUG1("HALHSMCI_Init\n");

  PMC->PMC_PCER0 = BIT(ID_HSMCI); // HSMCI enable

  HSMCI->HSMCI_WPMR = 0x4D434900; // Disables the Write Protection

  uint32_t mask = BIT(BIT_SD_DAT2) | BIT(BIT_SD_DAT3) | BIT(BIT_SD_CMD) | BIT(BIT_SD_CLK) | BIT(BIT_SD_DAT0) | BIT(BIT_SD_DAT1);

  PIOA->PIO_PUER = mask;  // Pullup Enable Register
  PIOA->PIO_MDDR = mask;  // Multi-Drive Disable Register
  PIOA->PIO_PDR = mask;   // PIO Disable Register
  PIOA->PIO_ABCDSR[0] &= ~mask;
  PIOA->PIO_ABCDSR[1] |= mask;

  HSMCI->HSMCI_CR = HSMCI_CR_SWRST; // Software Reset

  HSMCI->HSMCI_CR =
    HSMCI_CR_MCIEN | // Multi-Media Interface Enable
    HSMCI_CR_PWSDIS; // Power Save Mode Disable

  //const uint8_t CLKDIV = 0xFF;  // najwolniejszy zegar
  //const uint32_t MCI_CLKDIV_MASK = (uint32_t)CLKDIV;  //64MHz/(2x(CLKDIV+1)) = 125 kHz
  //const uint32_t MCI_BLKLEN_MASK = (uint32_t)(BLOCK_SIZE << 16);

  #if USE_DMA
    HSMCI->HSMCI_MR =
      HSMCI_MR_CLKDIV(149) |
      HSMCI_MR_PWSDIV(7) |
      HSMCI_MR_RDPROOF |
      HSMCI_MR_WRPROOF |
      HSMCI_MR_FBYTE;
  #else
    HSMCI->HSMCI_MR =
      HSMCI_MR_CLKDIV(149) |  // Clock Divider: MCK / (2 * (CLKDIV + 1)) = 400kHz
      HSMCI_MR_PWSDIV(7) |    // Power Saving Divider
      HSMCI_MR_RDPROOF |      // Read Proof Enable
      HSMCI_MR_WRPROOF;       // Write Proof Enable
  #endif
  //DEBUG2("MCI_MR:%08X\n", HSMCI->HSMCI_MR);

  HSMCI->HSMCI_BLKR = HSMCI_BLKR_BCNT(512);  // Blocklänge = 512 byte

  HSMCI->HSMCI_SDCR =
    HSMCI_SDCR_SDCSEL_SLOTA | // Slot A is selected.
    HSMCI_SDCR_SDCBUS_4;      // SDCard/SDIO Bus Width is 4

  HSMCI->HSMCI_DTOR =
    HSMCI_DTOR_DTOCYC(2) |     // Data Timeout Cycle Number
    HSMCI_DTOR_DTOMUL_1048576; // Data Timeout Multiplier is DTOCYC * 1048576

  HSMCI->HSMCI_CSTOR =
    HSMCI_CSTOR_CSTOCYC(2) |     // Data Timeout Cycle Number
    HSMCI_CSTOR_CSTOMUL_1048576; // Data Timeout Multiplier is DTOCYC * 1048576

  return CardInit();
}
/*---------------------------------------------------------------------------*/
void HALHSMCI_Deinit(void)
{
  DEBUG1("HALHSMCI_Deinit\n");
  HSMCI->HSMCI_CR = 0;
  PMC->PMC_PCDR0 = BIT(ID_HSMCI); // HSMCI disable
}
/*---------------------------------------------------------------------------*/
int16_t HALHSMCI_WriteBlocks(const void *buffer, uint32_t lba, uint32_t blocksToWrite)
{
  #if USE_DMA
    uint8_t *pBuf = (uint8_t*) buffer;
    while (blocksToWrite > MAX_DMA_BLOCKS) {
      int16_t result = WriteBlocksDMA(pBuf, lba, MAX_DMA_BLOCKS);
      pBuf += BLOCK_SIZE * MAX_DMA_BLOCKS;
      blocksToWrite -= MAX_DMA_BLOCKS;
      lba += MAX_DMA_BLOCKS;
    }
    if (blocksToWrite) {
      int16_t result = WriteBlocksDMA(pBuf, lba, blocksToWrite);
      if (result < 0) return result;
    }
  #else
    uint8_t *pBuf = (uint8_t*) buffer;
    while (blocksToWrite--) {
      int16_t result = WriteBlock(pBuf, lba);
      if (result < 0)return result;
      pBuf += BLOCK_SIZE;
      lba++;
    }
  #endif
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HALHSMCI_ReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead)
{
  #if USE_DMA
    uint8_t *pBuf = (uint8_t*) buffer;
    while (blocksToRead > MAX_DMA_BLOCKS) {
      int16_t result = ReadBlocksDMA(pBuf, lba, MAX_DMA_BLOCKS);
      if (result < 0) return result;
      pBuf += BLOCK_SIZE * MAX_DMA_BLOCKS;
      blocksToRead -= MAX_DMA_BLOCKS;
      lba += MAX_DMA_BLOCKS;
    }
    if (blocksToRead) {
      int16_t result = ReadBlocksDMA(pBuf, lba, blocksToRead);
      if (result < 0) return result;
    }
  #else
    uint8_t *pBuf = (uint8_t*) buffer;
    while (blocksToRead--) {
      int16_t result = ReadBlock(pBuf, lba);
      if (result < 0)return result;
      pBuf += BLOCK_SIZE;
      lba++;
    }
  #endif
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HALHSMCI_GetSizeInfo(uint32_t *availableBlocks, uint16_t *sizeOfBlock)
{
  *availableBlocks = hsmci.numberOfBlocks * (hsmci.actualBlockSize / BLOCK_SIZE);
  *sizeOfBlock = BLOCK_SIZE;  //return always the same block size
}
/*---------------------------------------------------------------------------*/
