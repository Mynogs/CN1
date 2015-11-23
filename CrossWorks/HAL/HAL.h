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
#ifndef HAL__H
#define HAL__H
/*===========================================================================
  HAL
  ===========================================================================*/
#include "Common.h" /*lint -e537 */
#ifdef HAS_GYR_DISPLAY
  #include "HALGYRDisplay.h"
#endif
/*---------------------------------------------------------------------------*/
#define SWAP(A, B) ((A) ^= (B), (B) ^= (A),  (A) ^= (B))
/*---------------------------------------------------------------------------*/
/*!
 * \brief Global interrupt disable
 */
#define GlobalInterruptsDisable() __asm volatile("cpsid i")
/*!
 * \brief Global interrupt enable
 */
#define GlobalInterruptsEnable() __asm volatile("cpsie i")
/*---------------------------------------------------------------------------*/
/*!
 * \brief System fatal id's
 */
#define FATAL_DATA_ABORT_EXECPTION         1
#define FATAL_HARD_FAULT_EXECPTION         2
#define FATAL_SLOW_CLOCK_ACTIVE            3
#define FATAL_STACK_TOO_SMALL              4
#define FATAL_MAIN_LOOP_ENDS               5
#define FATAL_ASSERT                       6
#define FATAL_ETHERNT_FAIL                 7
#define FATAL_OUT_OF_MEMORY                8
#define FATAL_ABORT                        9
#define FATAL_FLASH_WRITE                 10
#define FATAL_NO_CRYPTO_CHIP              11
#define FATAL_ZERO_CONFIG_FAIL            12
#define FATAL_NO_DRIVE_FOUND              13
#define FATAL_NO_STARTUP                  14

#define FATAL_LIS_NOT_FOUND               50
/*---------------------------------------------------------------------------*/
/*!
 * \brief SAM flash macros
 */

#define WP 0x01 //Write page
#define ES 0x11 //Erase sector
/*---------------------------------------------------------------------------*/
#define RESULT_OK                          0
#define RESULT_ERROR                      -1
#define RESULT_ERROR_BAD_HANDEL           -2
#define RESULT_ERROR_BAD_SIZE             -3
#define RESULT_ERROR_TIMEOUT              -4
#define RESULT_ERROR_FORBIDDEN            -5
#define RESULT_ERROR_NOT_AVAIL            -6
#define RESULT_ERROR_NO_ACK               -7

#define RESULT_HAL_ERROR_RESOURCE_IN_USE  -10
#define RESULT_HAL_ERROR_UNKNOWN_PIN_NAME -11
#define RESULT_HAL_ERROR_IS_OPEN          -12
#define RESULT_HAL_ERROR_IS_NOT_OPEN      -13
#define RESULT_HAL_ERROR_WRONG_PARAMETER  -14
#define RESULT_HAL_ERROR_WRONG_CHANNEL    -15
#define RESULT_HAL_ERROR_NO_MORE_CHANNELS -16
#define RESULT_HAL_ERROR_DEVICE_NOT_FOUND -17
#define RESULT_HAL_ERROR_DEVICE_NOT_WORK  -18

/*---------------------------------------------------------------------------*/
#define RESCOURCES_COUNT (26)
/*---------------------------------------------------------------------------*/
#define RESOURCE_OWNER_FREE           0
#define RESOURCE_OWNER_PIN            1
#define RESOURCE_OWNER_SERIAL_0       2
#define RESOURCE_OWNER_SERIAL_1       3
#define RESOURCE_OWNER_SERIAL_2       4
#define RESOURCE_OWNER_SERIAL_0_TXEN  5
#define RESOURCE_OWNER_SPI            6
#define RESOURCE_OWNER_I2C            7
#define RESOURCE_OWNER_LIS            8
#define RESOURCE_OWNER_EVE            9
#define RESOURCE_OWNER_RGY           10
#define RESOURCE_OWNER_ADC           11
/*---------------------------------------------------------------------------*/
/*!
 * \brief Port definitions
 */
/*---------------------------------------------------------------------------*/
/* Port A */
#ifdef BOARD_STAMP
  #define BIT_CRYP_SDA_RX 5
  #define BIT_CRYP_SDA_TX 6
#endif
#define BIT_ETH_CS    11
#define BIT_MISO      12
#define BIT_MOSI      13
#define BIT_SCK       14
#define BIT_SD_DET    25
#define BIT_SD_DAT2   26
#define BIT_SD_DAT3   27
#define BIT_SD_CMD    28
#define BIT_SD_CLK    29
#define BIT_SD_DAT0   30
#define BIT_SD_DAT1   31
/*---------------------------------------------------------------------------*/
/* Port B */
#define BIT_ETH_RES   13
#define BIT_ETH_INT   14
/*---------------------------------------------------------------------------*/
typedef enum SDCardState {
  csUnknown,
  csRemoved   = 4, // 0b100  No card in the slot
  csInserted  = 5, // 0b101  There is a card in the slot
  csRemoving  = 6, // 0b110  Card was just removed
  csInserting = 7  // 0b111  Card was just inserted
} SDCardState;
#define SDCARD_STATE_CARD_MASK  BIT(0)
#define SDCARD_STATE_EVENT_MASK BIT(1)
#define SDCARD_STATE_VALID_MASK BIT(2)
/*---------------------------------------------------------------------------*/
typedef struct SoftCounter {
  int16_t pinId;
  int64_t count;
} SoftCounter;
/*---------------------------------------------------------------------------*/
/*!
 * \brief HAL_ data structure
 */
typedef struct HAL {
  volatile uint32_t preGuard;               // Memory guard
  volatile uint64_t tickMS;                 // Tick counter. 1ms steps 
  volatile uint16_t tick1000MS;             // Tick counter mod 1000. 1ms steps, rollover after 1s 
  volatile uint32_t timeSinceBootS;         // Second counter. 1s steps, rollover after 136 years 
  struct {                                  // Timer for sys.onTick() function 
    uint32_t now;
    uint32_t last;
  } tick;
  #ifdef HAS_WATCHDOG
    volatile uint32_t watchdogAutoServeKey; // If it has the correkt key watchdog will server every second 
  #endif
  volatile uint32_t sysLEDPattern;          // Sys LED pattern 
  volatile uint32_t delayTimer;             // Delay timer for Delay function 
  volatile SDCardState sdCardState;
  struct {
    enum {
      dsOff = 0,                            // Uncalibrated 
      dsStart = 1,
      dsStop = 11,
      dsFinished = 12                       // Calibration finished 
    } state;
    uint32_t count;
    uint32_t calibrate10;
  } volatile delay;
  uint8_t resourceState[RESCOURCES_COUNT];
  volatile bool interruptTrigger;
  #ifdef HAS_GYR_DISPLAY
    volatile GYRDisplay gyrDisplay;
  #endif
  SoftCounter counter[8];
  volatile uint32_t postGuard;              // Memory guard 
} HAL;
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
const char* HAL_ErrorToString(int16_t result);
/*---------------------------------------------------------------------------*/
/*!
 * \brief System fatal function. Red led will flash fast. Very simple
 * function. Can be call very early in system initalisation.
 * \param id Fatal id. Use FATAL_xxx macros
 */
void HAL_SysFatal(uint8_t id);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the stack usage in bytes. On startup stack is filled with 0xCC.
 * This function checks how many of the 0xCC are gone.
 * \note Current stach size is &__stack_end__ - &__stack_start__
 * \return Stack usage
 */
uint32_t HAL_GetStackUsage(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Internal assert function.
 * \param text Pointer to static assert text
 * \param file Pointer to static file name
 * \param line The assert line number
 * \note Don't use directly,  always use the assert macro!
 */
void ____assert(char *text, char *file, int line);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Assert macro. If the condition is false the assert condition is
 * send to DEBUG output. Then the system enters HAL_SysFatal(FATAL_ASSERT)
 * \param COND contion to test
 */
#define assert(COND) if (!(COND)) ____assert("Assertion " #COND, __FILE__, __LINE__)
/*---------------------------------------------------------------------------*/
void abort();
/*---------------------------------------------------------------------------*/
/*!
 * \brief Set the LED pattern
 * Every bit switch the LED on or off  for 16ms,  high to low bit.
 * This repates after 1s.
 * So the LED active phaes is 32 x 16ms = 512ms. For the remaining time
 * bit 0 set the LED status.
 * Samples:
 *    -1 : Always on
 *     0 : Always off
 *   10b : on: 16ms  (1:999)
 * 1010b : Two flashes for 16ms
 *     1 : on: 496ms (50:50)
 */
uint32_t HAL_SysLED(uint32_t pattern);
/*---------------------------------------------------------------------------*/
/*!
 * \briefe Reboot the system
 */
void HAL_Reboot(void);
/*---------------------------------------------------------------------------*/
/*! 
 * \brief Disable the watchdog.
 */         
#ifdef HAS_WATCHDOG
  int16_t HAL_WatchdogInit(double timeS);
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  #define HAL_WATCHDOG_SERVE_KEY 0xBADC0DED
  void HAL_WatchdogServe(uint32_t key);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Check for a tick
 * \note This function must work fast as possible. It will called often!
 */
bool HAL_TickIsTicked(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the time between the last ticks in seconds
 */
double HAL_TickGetElapsedTime(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief New implemtation of ATMELS in Application Programming (IAP) Feature.
 * For details see SAM3S manual.
 * This function builds the correct longword for flash programming. Then the
 * function call _IAP function in SRAM.
 * \param flashCommand The flash programming command
 * \param flashSectorNumber The sector number
 * \return EEFC_FSR register
 * \note Interrupt are temporary disabled.
 */
uint32_t IAP(uint8_t flashCommand, uint16_t flashSectorNumber);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the seconds since board was booted
 */
uint32_t HAL_GetSecondsSinceBoot(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Do a delay of multiple ms.
 * \param time Time to delay in ms
 * \note Uses system tick interrupt
 */
void HAL_DelayMS(uint32_t timeMS);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Do a delay of multiple 탎.
 * \param time Time to delay in 탎
 * \note Uses calibrated for loop
 */
void HAL_DelayUS(uint32_t timeUS);
/*---------------------------------------------------------------------------*/
SDCardState HAL_GetSDCardState(void);
/*---------------------------------------------------------------------------*/
/*!
 * \brief Get the temperature of the chip temperatur sensor
 * \note The temperture sensor is on the CPU chip. The measued temperatur is
 * normally more thenn 20째C higher as the ambient temeperture
 * \return Temperature in 1/10 째C
 */
int16_t HAL_TemperatureSensorGet(void);
/*---------------------------------------------------------------------------
  Flash
  --------------------------------------------------------------------------- */
/*!
 * \brief Calculate the CRC32 of the application flash memory.
 * \result The CRC32 value
 * \note Application flash memory is the range from __FLASH_segment_start__
 * to __FLASH_segment_end__ minus one.
 */
uint32_t HAL_FlashCRC(void);
/*---------------------------------------------------------------------------
  RAM
  ---------------------------------------------------------------------------*/
/*!
 * \brief Do a RAM test
 * \result 1: Success else 0
 * \note Not implemented yet!
 */
uint8_t HAL_RAMTest(void);
/*---------------------------------------------------------------------------
  SPI
  ---------------------------------------------------------------------------*/
int16_t HAL_SPIEthWriteRead(uint8_t send);
/*---------------------------------------------------------------------------
  ETH
  ---------------------------------------------------------------------------*/
void HAL_EthReset(void);
void HAL_EthCS1(void);
void HAL_EthCS0(void);
/*---------------------------------------------------------------------------
  ATSHA204
  ---------------------------------------------------------------------------*/
//uint8_t HAL_SHA204Send(uint8_t sendCount, uint8_t *send);
//uint8_t HAL_SHA204Recv(uint8_t recvCount, uint8_t *recv);
void HAL_SHA204SignalPin(uint8_t high);
/*!
 * \brief Send wake signal
 */
#ifdef HAS_ATSHA204
  //void HAL_ATSHA204Wake(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Exchange data
 * \param send Data to send,  could be 0
 * \param sendCount How many bytes to send,  could be 0
 * \param recv Receive buffer,  could be 0
 * \param recvCount How many bytes to recive,  could be 0
 * \return Number of bytes recived. If lower recvCount a timeout occured.
 */
#ifdef HAS_ATSHA204
  int8_t HAL_ATSHA204IO(uint8_t *send, uint8_t sendCount, uint8_t *recv, uint8_t recvCount, uint8_t recvTimeoutMS);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief The "printf" character output function
 * \return Always 1
 * \note Uses a 80 byte output buffer. This buffer is send to debug_printf
 * when a LF ocures or the buffer is full. If DEBUG_SHOW_STDOUT is defined.
 */
int putchar(int __c);
/*---------------------------------------------------------------------------
  Pin
  ---------------------------------------------------------------------------*/
/*!
 * \brief Get all pin names
 */
const char* HAL_PinGetNames(void);
/*!
 * \brief Convert a pin name to a id number
 * \param name The pin name
 * \return if >= 0 for a valid pin name else error
 */
int16_t HAL_PinNameToId(const char *name);
/*!
 * \brief Configurate the pin
 * \param id The pin id
 * \param config Config name like "RESET", "INPUT", "HIGH", "LOW", "PULLUP", "PULLDOWN"
 * \return 0 for a valid configration
 */
int16_t HAL_PinConfig(int16_t pinId, const char *config, uint8_t configNumber);
/*!
 * \brief Pin id to Pio struct
 */
Pio* HAL_GetPinPio(int16_t pinId);
/*!
 * \brief Pin id to bit mask
 */
uint32_t HAL_GetPinMask(int16_t pinId);
/*!
 * \brief Set the pin to high or low. Works only for output pins
 * \param id The pin id
 * \param high The pin value
 */
void HAL_PinSet(int16_t pinId, bool high);
/*!
 * \brief Get the pin input. Works only for input pins
 * \param id The pin id
 * \return The pin value
 */
bool HAL_PinGet(int16_t pinId);
/*---------------------------------------------------------------------------
  Serial
  ---------------------------------------------------------------------------*/
/*!
 * \brief Open a serial channel
 * \param channel The channel number starts at 0
 * \return 0 for a valid channel
 */
int16_t HAL_SerialOpen(uint16_t channel);
/*!
 * \brief Close the channel
 * \param channel The channel number starts at 0
 */
void HAL_SerialClose(uint16_t channel);
/*!
 * \brief Configurate the channel
 * \param channel The channel number starts at 0
 * \param config Config name like "EVEN", "ODD", "MARK", "SPACE", "RS485", "RS232"...
 * \return 0 for a valid configration
 */
int16_t HAL_SerialConfig(uint16_t channel, const char *config, uint8_t configNumber);
/*!
 * \brief Send bytes over the channel
 * \param channel The channel number starts at 0
 * \param send Date to send
 * \param size Number of bytes to send
 */
int32_t HAL_SerialSend(uint16_t channel, const uint8_t *send, uint32_t size);
/*!
 * \brief Recv bytes from the channel
 * First set recv and/or size to 0 to get the number of avail bytes.
 * Then allocate buffer and get they bytes with valid recv and size.
 * \param channel The channel number starts at 0
 * \param recv Receive buffer
 * \param size Number of bytes to get.
 * \return If >= 0 the number of bytes else error code
 */
int32_t HAL_SerialRecv(uint16_t channel, uint8_t *recv, uint32_t size);
/*!
 * \brief Convert an error code to string
 * \param recv The error code
 * \return The error string
 */
char* HAL_SerialErrorToString(int8_t result);
/*---------------------------------------------------------------------------
  SPI
  ---------------------------------------------------------------------------*/
/*!
 * \brief Open a SPI channel
 * \param channel The channel number starts at 0
 * \return 0 for a valid channel
 */
int16_t HAL_SPIOpen(uint16_t channel);
/*!
 * \brief Close the channel
 * \param channel The channel number starts at 0
 */
void HAL_SPIClose(uint16_t channel);
/*!
 * \brief Configurate the channel
 * \param channel The channel number starts at 0
 * \param config Config name
 * \return 0 for a valid configration
 */
int16_t HAL_SPIConfig(uint16_t channel, const char *config, uint8_t configNumber);
/*!
 * \brief Send and receive bytes over the channel
 * \param channel The channel number starts at 0
 * \param send Date to send
 * \param size Number of bytes to send
 */
int16_t HAL_SPISendRecvByte(uint16_t channel, uint8_t send);
/*!
 * \brief Send and receive bytes over the channel
 * \param channel The channel number starts at 0
 * \param send Date to send
 * \param size Number of bytes to send
 */
int32_t HAL_SPISendRecv(uint16_t channel, uint8_t *buffer, uint32_t size);
/*!
 * \brief Send bytes over the channel
 * \param channel The channel number starts at 0
 * \param send Date to send
 * \param size Number of bytes to send
 */
int32_t HAL_SPISend(uint16_t channel, const uint8_t *send, uint32_t size);
/*!
 * \brief Receive bytes over the channel
 * \param channel The channel number starts at 0
 * \param send Date to send
 * \param size Number of bytes to send
 */
int32_t HAL_SPIRecv(uint16_t channel, uint8_t *recv, uint32_t size);
/*---------------------------------------------------------------------------
  I2C
  ---------------------------------------------------------------------------*/
/*!
 * \brief Open a I2C channel
 * \param channel The channel number starts at 0
 * \return 0 for a valid channel
 */
#ifdef HAS_I2C
  int16_t HAL_I2COpen(uint16_t channel);
#endif
/*!
 * \brief Close the channel
 * \param channel The channel number starts at 0
 */
#ifdef HAS_I2C
  void HAL_I2CClose(uint16_t channel);
#endif
/*!
 * \brief Configurate the channel
 * \param channel The channel number starts at 0
 * \param config Config name like
 * \return 0 for a valid configration
 */
#ifdef HAS_I2C
  int16_t HAL_I2CConfig(uint16_t channel, const char *config);
#endif
/*!
 * \brief Does a device exist?
 * \param channel The channel number starts at 0
 * \param addr I2C address
 * \return 0 for a valid configration
 */
#ifdef HAS_I2C
int16_t HAL_I2CExist(uint16_t channel, uint8_t addr);
#endif

#ifdef HAS_I2C  
  int16_t HAL_I2CSend(uint16_t channel, uint8_t addr, uint32_t internalAddr, uint8_t internalAddrSize, uint8_t *send, uint16_t size);
#endif

#ifdef HAS_I2C
  int16_t HAL_I2CRecv(uint16_t channel, uint8_t addr, uint32_t internalAddr, uint8_t internalAddrSize, uint8_t *recv, uint16_t size);
#endif

/*---------------------------------------------------------------------------
  LIS
  ---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_LIS
  int16_t HAL_LISOpen(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_LIS
  void HAL_LISClose(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Reset the accel (LIS) by switching of the power
 */
#ifdef HAS_LIS
  void HAL_LISReset(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Low level function to send 16bit value to the LIS
 * \param send The value to send
 */
#ifdef HAS_LIS
  int16_t HAL_LISWrite16Read8(uint8_t send1, uint8_t send2);
#endif
/*---------------------------------------------------------------------------
  EVE
  ---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  int16_t HAL_EVEOpen(uint16_t channel);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  void HAL_EVEClose(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  void HAL_EVESetSpeed(uint32_t baud);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  void HAL_EVETransferBegin(void);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  uint8_t HAL_EVETransfer(uint8_t send);
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief
 */
#ifdef HAS_EVE
  void HAL_EVETransferEnd(void);
#endif
/*---------------------------------------------------------------------------
  RGY Display
  ---------------------------------------------------------------------------*/
#ifdef HAS_RGY_DISPLAY
void HAL_RGYSet(uint8_t *patternRed, uint8_t *patternGreen);
#endif
/*---------------------------------------------------------------------------
  Soft PWM
  ---------------------------------------------------------------------------*/
int16_t HAL_SoftPWMInit(uint32_t baseClockHz, uint16_t channelCount);
void HAL_SoftPWMDeinit(void);
int16_t HAL_SoftPWMConfig(uint16_t channelNumber, int16_t pinId);
int16_t HAL_SoftPWMSet(uint16_t channelNumber, uint32_t intervalLowNS, uint32_t intervalHighNS);
/*---------------------------------------------------------------------------
  Soft counter
  ---------------------------------------------------------------------------*/
void HAL_SoftCounterInit(void);
void HAL_SoftCounterDeinit(void);
int16_t HAL_SoftCounterConfig(int16_t pinId, const char *config, uint8_t configNumber);
int16_t HAL_SoftCounterGet(int16_t pinId, int64_t *actual, int64_t *set);
/*---------------------------------------------------------------------------
  ADC
  ---------------------------------------------------------------------------*/
int16_t HAL_ADCInit(bool autoCalibrate);
void HAL_ADCDeinit(void);
/*
 *! \briew Config the adc channel
 * GAIN1, GAIN2, GAIN4, BIPOLAR, UNIPOLAR
 */
int16_t HAL_ADCConfig(uint16_t channelNumber, const char *config, uint8_t configNumber);
int16_t HAL_ADCGet(uint16_t channelNumber, float *value);
int16_t HAL_ADCGetAll(float *values);
/*---------------------------------------------------------------------------
  HAL
  ---------------------------------------------------------------------------*/
/*!
 * \brief Init the HAL
 */
void HAL_Init(void);
/*---------------------------------------------------------------------------*/
#endif

