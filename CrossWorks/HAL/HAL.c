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
#include "HAL.h"
#include "HALW5500.h"
#include <ctype.h>
#include "../SysLog.h"
#include "../Modifyer.h"
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
#ifndef INITIALIZE_STACK
  #error INITIALIZE_STACK
#endif
/*---------------------------------------------------------------------------
  HAL
  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
extern uint8_t __fast_start__, __fast_end__;
extern uint8_t __data_start__, __data_end__;
extern uint8_t __bss_start__, __bss_end__;
extern uint8_t __stack_start__, __stack_end__;
extern uint8_t __heap_start__, __heap_end__;
extern uint8_t __FLASH_segment_start__, __FLASH_segment_end__;
extern uint8_t __RAM_segment_start__, __RAM_segment_end__;
/*---------------------------------------------------------------------------*/
HAL hal;
/*---------------------------------------------------------------------------*/
#define SERIAL_MODE_ISOPEN    (1 << 0)
#define SERIAL_MODE_ISDMX     (1 << 1)
#define SERIAL_MODE_ERROR     (1 << 2)
/*---------------------------------------------------------------------------*/
typedef struct Serial {
  uint8_t modes;     // See SERIAL_MODE_xxx
  volatile uint16_t timeoutMS;
  struct {
    struct {
      uint8_t *buffer;
      uint16_t top;
    } a, b;
    uint16_t size;
  } volatile recv;
} Serial;
Serial serial[3]; // USART1,  UART0,  UART1
/*---------------------------------------------------------------------------*/
typedef struct I2C {
  bool isOpen;
  uint32_t CWGR;
  uint32_t timeoutUS;
} I2C;
I2C i2c;
/*---------------------------------------------------------------------------*/
/*
typedef struct SerialPeripheralInterface {
  bool isOpen;
  / *
  struct {
    uint8_t buffer[64];
    uint8_t top;
  } volatile recv;
  * /
} SerialPeripheralInterface;
SerialPeripheralInterface spi;
*/
/*---------------------------------------------------------------------------*/
#ifdef REDIRECT_STDOUT_TO_DEBUG
  volatile uint8_t putcharFlushTimer = 0;
#endif
/*---------------------------------------------------------------------------
  Rescource manager
  ---------------------------------------------------------------------------*/
#define PINID(PORT, PIN) (((((PORT) - 'A') & 1) << 5) | (PIN))
/*---------------------------------------------------------------------------*/
const char* HAL_ErrorToString(int16_t result)
{
  switch (result) {
    case RESULT_OK                         : return "OK";
    case RESULT_ERROR                      : return "ERROR";
    case RESULT_ERROR_BAD_HANDEL           : return "ERROR_BAD_HANDEL";
    case RESULT_ERROR_BAD_SIZE             : return "ERROR_BAD_SIZE";
    case RESULT_ERROR_TIMEOUT              : return "ERROR_TIMEOUT";
    case RESULT_HAL_ERROR_RESOURCE_IN_USE  : return "HAL_ERROR_RESOURCE_IN_USE";
    case RESULT_HAL_ERROR_UNKNOWN_PIN_NAME : return "HAL_ERROR_UNKNOWN_PIN_NAME";
    case RESULT_HAL_ERROR_IS_OPEN          : return "HAL_ERROR_IS_OPEN";
    case RESULT_HAL_ERROR_IS_NOT_OPEN      : return "HAL_ERROR_IS_NOT_OPEN";
    case RESULT_HAL_ERROR_WRONG_PARAMETER  : return "HAL_ERROR_WRONG_PARAMTER";
    case RESULT_HAL_ERROR_WRONG_CHANNEL    : return "HAL_ERROR_WRONG_CHANNEL";
    case RESULT_HAL_ERROR_NO_MORE_CHANNELS : return "HAL_ERROR_NO_MORE_CHANNELS";
    case RESULT_HAL_ERROR_DEVICE_NOT_FOUND : return "HAL_ERROR_DEVICE_NOT_FOUND";
    case RESULT_HAL_ERROR_DEVICE_NOT_WORK  : return "HAL_ERROR_DEVICE_NOT_WORK";
  }
  return "UNKNOWN";
}
/*---------------------------------------------------------------------------
  Rescources
  ---------------------------------------------------------------------------*/
/*!
 * \brief Resource id table
 */
static const int16_t Resources[RESCOURCES_COUNT] = {
  // Port A */
  PINID('A', 0), PINID('A', 1), PINID('A', 2), PINID('A', 3), PINID('A', 4),
  PINID('A', 7), PINID('A', 8), PINID('A', 9), PINID('A', 10),
  PINID('A', 15), PINID('A', 16), PINID('A', 17), PINID('A', 18), PINID('A', 19), PINID('A', 20), PINID('A', 21), PINID('A', 22), PINID('A', 23), PINID('A', 24),
  // Port B */
  PINID('B', 0), PINID('B', 1), PINID('B', 2), PINID('B', 3),
  PINID('B', 10), PINID('A', 11), PINID('A', 12),
};
/*---------------------------------------------------------------------------*/
static int16_t ResourceIdToIndex(int16_t pinId)
{
  for (uint8_t i = 0; i < RESCOURCES_COUNT; i++)
    if (Resources[i] == pinId)
      return i;
  return RESULT_HAL_ERROR_UNKNOWN_PIN_NAME;
}
/*---------------------------------------------------------------------------*/
static int16_t ResourceLock2(uint8_t id, int16_t pinIdStart, int16_t pinIdEnd)
{
  for (int16_t pinId = pinIdStart; pinId <= pinIdEnd; pinId++) {
    int16_t number = ResourceIdToIndex(pinId);
    if (number < 0)
      return number;
    if (hal.resourceState[number] != RESOURCE_OWNER_FREE)
      return RESULT_HAL_ERROR_RESOURCE_IN_USE;
  }
  for (int16_t pinId = pinIdStart; pinId <= pinIdEnd; pinId++)
    hal.resourceState[ResourceIdToIndex(pinId)] = id;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int16_t ResourceLock1(uint8_t id, int16_t pinId)
{
  return ResourceLock2(id, pinId, pinId);
}
/*---------------------------------------------------------------------------*/
void ResourceUnlock(uint8_t id)
{
  for (uint8_t i = 0; i < RESCOURCES_COUNT;i++)
    if (hal.resourceState[i] == id) hal.resourceState[i] = RESOURCE_OWNER_FREE;
}
/*---------------------------------------------------------------------------*/
void ResourceUnlockAll(void)
{
  for (uint8_t i = 0; i < RESCOURCES_COUNT;i++)
    hal.resourceState[i] = RESOURCE_OWNER_FREE;
}
/*---------------------------------------------------------------------------

  ---------------------------------------------------------------------------*/
/*!
 * \brief Switch LED on or off, and check sd-card det signal.
 * \param mode 0: LED off, 1: LED on, 2:Detect sd-card
 * \return Detect status from the call *before*
 */
static bool LEDAndDET(uint8_t mode)
{
  bool detect = (PIOA->PIO_PDSR & BIT(BIT_SD_DET)) == 0; // PIO Pin Data Status Register, card inserted: low
  if (mode >= 2) {
    PIOA->PIO_ODR = BIT(BIT_SD_DET);  // Output Disable Register
    return detect;
  } else {
    PIOA->PIO_OER = BIT(BIT_SD_DET);  // Output Enable Register
    if (mode)
      PIOA->PIO_SODR = BIT(BIT_SD_DET); // Set output Data Register
    else
      PIOA->PIO_CODR = BIT(BIT_SD_DET); // Clear output Data Register
  }
  return detect;
}
/*---------------------------------------------------------------------------*/
void HAL_SysFatal(uint8_t id)
{
  char s[40];
  snprintf(s, sizeof(s), "CN: Fatal %u ", id);
  switch (id) {
    case FATAL_DATA_ABORT_EXECPTION : strcat(s, "data abord exeption"); break;
    case FATAL_HARD_FAULT_EXECPTION : strcat(s, "hard fault exeption"); break;
    case FATAL_STACK_TOO_SMALL      : strcat(s, "stack too small"); break;
    case FATAL_MAIN_LOOP_ENDS       : strcat(s, "main loop end"); break;
    case FATAL_ASSERT               : strcat(s, "assert"); break;
    case FATAL_ETHERNT_FAIL         : strcat(s, "ethernet fail"); break;
    case FATAL_OUT_OF_MEMORY        : strcat(s, "out of memory"); break;
    case FATAL_ZERO_CONFIG_FAIL     : strcat(s, "zeroconf fail"); break;
    case FATAL_NO_DRIVE_FOUND       : strcat(s, "no drive found"); break;
    case FATAL_NO_STARTUP           : strcat(s, "no startup file"); break;
  }
  SysLog_Config(IP_BROADCAST, SYSLOG_PORT);
  SysLog_Send(SYSLOG_KERN | SYSLOG_EMERG, s);
  SysLog_Config(IP_BROADCAST, SYSLOG_NOGS_PORT);
  SysLog_Send(SYSLOG_KERN | SYSLOG_EMERG, s);
  DEBUG(s);
  GlobalInterruptsDisable();
  LEDAndDET(1);
  puts("CN: Core dump");
  printf("CN: Stack usage:%u\n", HAL_GetStackUsage());
  CN_Dump();
  //Memory_Dump();
  for (;;) {
    for (uint16_t i = 0; i < 10000L; i++) 
      LEDAndDET(1);
    for (uint8_t i = 0; i < id; i++) {
      for (uint16_t i = 0; i < 100; i++) 
        LEDAndDET(0);
      for (uint16_t i = 0; i < 100; i++) 
        LEDAndDET(1);
    }
  }
}
/*---------------------------------------------------------------------------*/
static uint32_t HAL_GetAreaUsage(uint8_t *start, uint8_t *end)
{
  size_t size = end - start;
  size_t unused = 0;
  uint32_t *ptr = (uint32_t*) start;
  for (size_t i = 0; i < size / sizeof(void*); i++) {
    if (*ptr++ != 0xCCCCCCCC) 
      break;
    unused += 4;
  }
  return size - unused;
}
/*---------------------------------------------------------------------------*/
uint32_t HAL_GetStackUsage(void)
{
  return HAL_GetAreaUsage(&__stack_start__, &__stack_end__);
}
/*---------------------------------------------------------------------------*/
void ____assert(char *text, char *file, int line)
{
  DEBUG("assert %s in file %s, line %d\n", text, file, line);
  HAL_SysFatal(FATAL_ASSERT);
}
/*---------------------------------------------------------------------------*/
void abort()
{
  HAL_SysFatal(FATAL_ABORT);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief Hard fault handler
 */
extern void CN_TraceDump(void);

static void HardFaultHandler2(uint32_t *hardfaultArgs)
{
  GlobalInterruptsDisable();
  uint32_t r0 = hardfaultArgs[0];
  uint32_t r1 = hardfaultArgs[1];
  uint32_t r2 = hardfaultArgs[2];
  uint32_t r3 = hardfaultArgs[3];
  uint32_t r12 = hardfaultArgs[4];
  uint32_t lr = hardfaultArgs[5];
  uint32_t pc = hardfaultArgs[6];
  uint32_t psr = hardfaultArgs[7];
  uint32_t BFAR = *((volatile uint32_t*)(0xE000ED38));
  uint32_t CFSR = *((volatile uint32_t*)(0xE000ED28));
  uint32_t HFSR = *((volatile uint32_t*)(0xE000ED2C));
  uint32_t DFSR = *((volatile uint32_t*)(0xE000ED30));
  uint32_t AFSR = *((volatile uint32_t*)(0xE000ED3C));
  debug_printf("\n\n[Hard fault handler]\n");
  debug_printf("R0 0x%08X\n", r0);
  debug_printf("R1 0x%08X\n", r1);
  debug_printf("R2 0x%08X\n", r2);
  debug_printf("R3 0x%08X\n", r3);
  debug_printf("R12 0x%08X\n", r12);
  debug_printf("R14 [LR] 0x%08X\n", lr);
  debug_printf("R15 [PC] 0x%08X\n", pc);
  debug_printf("PSR = 0x%08X\n", psr);
  debug_printf("BFAR = 0x%08X\n", BFAR);
  debug_printf("CFSR = 0x%08X\n", CFSR);
  debug_printf("HFSR = 0x%08X\n", HFSR);
  debug_printf("DFSR = 0x%08X\n", DFSR);
  debug_printf("AFSR = 0x%08X\n", AFSR);
  //debug_printf ("SCB_SHCSR = %08X\n",  SCB->SHCSR);

  if (HFSR & BIT(30)) {
    debug_printf("Forced hard fault");
    if (CFSR & BIT(16)) debug_printf(" UNDEFINSTR");
    if (CFSR & BIT(17)) debug_printf(" INVSTATE");
    if (CFSR & BIT(18)) debug_printf(" INVPC");
    if (CFSR & BIT(19)) debug_printf(" NOCP");
    if (CFSR & BIT(24)) debug_printf(" UNALGINED");
    if (CFSR & BIT(25)) debug_printf(" DIBBYZERO");
    debug_printf("\n");
  }
  debug_printf("Stack usage %u\n", HAL_GetStackUsage());
  //__asm__("BKPT #01");
  CN_Dump();
  Memory_Dump();
  __asm__("BKPT #01");
  HAL_SysFatal(FATAL_HARD_FAULT_EXECPTION);
}
/*---------------------------------------------------------------------------
/*!
 * \brief This function handles Hard Fault exception
 */
void HardFault_Handler(void)
{
  __asm__(".global HardFault_Handler");
  __asm__(".extern HardFaultHandler");
  __asm__("TST LR, #4");
  __asm__("ITE EQ");
  __asm__("MRSEQ R0, MSP");
  __asm__("MRSNE R0, PSP");
  #ifdef STARTUP_FROM_RESET // TODO ??????
    __asm__("B HardFaultHandler2");
    for (;;);
  #else
    __asm__("B HardFaultHandler2");
  #endif
}
/*---------------------------------------------------------------------------*/
void HAL_Reboot(void)
{
  DEBUG("Reboot!\n");
  GlobalInterruptsDisable();
  #ifdef STARTUP_FROM_RESET
    RSTC->RSTC_CR = 0xA5000000 | BIT(2) | BIT(0);
    while (RSTC->RSTC_SR & BIT(17));
  #endif
  __asm__("B _start");
}
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  int16_t HAL_WatchdogInit(double timeS)
  {
    uint32_t timeMS = timeS * 1000;
    uint32_t time = timeMS * 256 / 1000;
    if (!time || (time > 0xFFF))
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    PMC->PMC_PCER0 = BIT(ID_WDT); // Enable watchdog 
    uint32_t mode = 
      WDT_MR_WDDBGHLT |  // WDT stops in debug state
      WDT_MR_WDRSTEN |   // Watchdog fault (underflow or error) activates the processor reset
      WDT_MR_WDD(time) |
      WDT_MR_WDV(time);
    WDT->WDT_MR = mode;
    if (WDT->WDT_MR != mode)
      return RESULT_ERROR_FORBIDDEN;
    hal.watchdogAutoServeKey = ~0xDECAFBAD; // Switch of watchdog auto serve every second 
    return 1;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_WATCHDOG
  void HAL_WatchdogServe(uint32_t key)
  {
    if ((hal.watchdogAutoServeKey == ~0xDECAFBAD) && (key == HAL_WATCHDOG_SERVE_KEY))
      WDT->WDT_CR = 0xA5000001;
  }
#endif
/*---------------------------------------------------------------------------*/
bool HAL_TickIsTicked(void)
{
  hal.tick.now = hal.tickMS / 10;
  return hal.tick.now > hal.tick.last;
}
/*---------------------------------------------------------------------------*/
double HAL_TickGetElapsedTime(void)
{
  double ellapsed = (double) (hal.tick.now - hal.tick.last) / 100.0;
  hal.tick.last = hal.tick.now;
  return ellapsed;
}
/*---------------------------------------------------------------------------*/
uint32_t HAL_SysLED(uint32_t pattern)
{
  uint32_t last = hal.sysLEDPattern;
  hal.sysLEDPattern = pattern;
  return last;
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief IAP (In Application Programming Feature)
 * \note This function is placed in RAM. Invoking the IAP is not allowed
 * from flash memory.
 */
static uint32_t __attribute__ ((long_call,  section (".fast"))) ____IAP(uint32_t command)
{
  while ((EFC0->EEFC_FSR & EEFC_FSR_FRDY) == 0) {}
  EFC0->EEFC_FCR = command;
  while ((EFC0->EEFC_FSR & EEFC_FSR_FRDY) == 0) {}
  return EFC0->EEFC_FSR;
}
/*---------------------------------------------------------------------------*/
uint32_t IAP(uint8_t flashCommand, uint16_t flashSectorNumber)
{
  uint32_t command = 0x5A000000 | ((uint32_t) flashSectorNumber << 8) | flashCommand;
  return ____IAP(command);
}
/*---------------------------------------------------------------------------*/
uint32_t HAL_GetSecondsSinceBoot(void)
{
  return hal.timeSinceBootS;
}
/*---------------------------------------------------------------------------*/
void HAL_DelayMS(uint32_t timeMS)
{
  hal.delayTimer = timeMS;
  while (hal.delayTimer) {}
}
/*---------------------------------------------------------------------------*/
#define DELAY_US(T) \
  for (hal.delay.count = 0; hal.delay.count < (T); hal.delay.count++) {}
#define DELAY_US_BREAK(T, B) \
  for (hal.delay.count = 0; hal.delay.count < (T); hal.delay.count++) {if (B) break;}
void HAL_DelayUS(uint32_t timeUS)
{
  GlobalInterruptsDisable();
  uint32_t top = timeUS * hal.delay.calibrate10 / 10000;
  DELAY_US(top);
  GlobalInterruptsEnable();
}
/*---------------------------------------------------------------------------*/
SDCardState HAL_GetSDCardState(void)
{
  SDCardState result = hal.sdCardState;
  hal.sdCardState &= ~SDCARD_STATE_EVENT_MASK;
  return result;
}
/*---------------------------------------------------------------------------
  SysTick
  ---------------------------------------------------------------------------*/
/*!
 * \brief Init the SAM3S system tick timer with 1ms rate
 * \note The calls the SysTick_Handler
 */
static void HAL_SysTickInit(void)
{
  // Defines from core-cm4.h, so only *_Pos and *_Msk defined
  SysTick->LOAD = (SystemCoreClock / 1000) - 1;
  SysTick->CTRL = 
    BIT(SysTick_CTRL_TICKINT_Pos) |  // Enables SysTick exception request
    BIT(SysTick_CTRL_ENABLE_Pos) |   // Enables the counter
    BIT(SysTick_CTRL_CLKSOURCE_Pos); // MCK

  uint32_t top = 100000;
  for (hal.delay.count = 0, hal.delay.state = dsStart; hal.delay.count < top; hal.delay.count++) {}
  //DEBUG("%u\n", hal.delay.calibrate10);
}
/*---------------------------------------------------------------------------*/
/*!
 * \brief SAM system tick handler. Called every 1ms
 */
extern void HALLIS_Server1MS(void); //TODO

void SysTick_Handler(void)
{
  hal.interruptTrigger = true;
  if (hal.delay.state) {
    if (hal.delay.state == dsStart) {
      hal.delay.count = 0;
      hal.delay.state++;
    } else if (hal.delay.state == dsStop) {
      hal.delay.calibrate10 = hal.delay.count;
      hal.delay.state++;
    } else if (hal.delay.state < dsFinished)
      hal.delay.state++;

  }
  hal.tickMS++;
  if (++hal.tick1000MS >= 1000) {
    hal.tick1000MS = 0;
    hal.timeSinceBootS++;
    if (hal.watchdogAutoServeKey == 0xDECAFBAD)
      WDT->WDT_CR = 0xA5000001;
  }
  if (hal.delayTimer) 
    hal.delayTimer--;
  if (serial[0].timeoutMS)
    serial[0].timeoutMS--;
  if (serial[1].timeoutMS)
    serial[1].timeoutMS--;
  if (serial[2].timeoutMS)
    serial[2].timeoutMS--;

  static uint32_t pattern = 0;
  if (hal.tick1000MS == 0) {
    LEDAndDET(2);
    pattern = hal.sysLEDPattern;
  } else if (hal.tick1000MS == 1) {
    bool detect = LEDAndDET(1);
    if ((hal.sdCardState == csUnknown) || ((hal.sdCardState & SDCARD_STATE_CARD_MASK) != detect)) {
      hal.sdCardState = SDCARD_STATE_VALID_MASK | SDCARD_STATE_EVENT_MASK | detect;
    }
  } else if (!(hal.tick1000MS & 0xF)) {
    LEDAndDET((pattern & 0x80000000) != 0);
    pattern <<= 1;
  }

  #ifdef DEBUG_SHOW_STDOUT
    if (putcharFlushTimer) 
      putcharFlushTimer--;
  #endif
  #ifdef HAS_LIS
    HALLIS_Server1MS();
  #endif
}
/*---------------------------------------------------------------------------
  RAM
  ---------------------------------------------------------------------------*/
uint8_t HAL_RAMTest(void)
{
  uint8_t Test(uint32_t *start, uint32_t *end, char *name)
  {
    DEBUG("RAM test %08X to %08X (%s)\n", start, end, name);
    register volatile uint32_t* ptr = start;
    while (ptr < end) {
      register uint32_t save = *ptr;
      *ptr = 0x00000000;
      if (*ptr != 0x00000000) return 0;
      *ptr = 0xFFFFFFFF;
      if (*ptr != 0xFFFFFFFF) return 0;
      *ptr = 0x55555555;
      if (*ptr != 0x55555555) return 0;
      *ptr = 0xAAAAAAAA;
      if (*ptr != 0xAAAAAAAA) return 0;
      *ptr = (uint32_t) ptr;
      if (*ptr != (uint32_t) ptr) return 0;
      *ptr++ = save;
    }
    return 1;
  }

  if (!Test((uint32_t*) &__fast_start__,  (uint32_t*) &__fast_end__, "fast")) return 0;
  if (!Test((uint32_t*) &__data_start__,  (uint32_t*) &__data_end__, "data")) return 0;
  if (!Test((uint32_t*) &__bss_start__,   (uint32_t*) &__bss_end__, "bss")) return 0;
  if (!Test((uint32_t*) &__stack_start__, (uint32_t*) &__stack_end__, "stack")) return 0;
  if (!Test((uint32_t*) &__heap_start__,  (uint32_t*) &__heap_end__, "heap")) return 0;
  if (!Test((uint32_t*) &__heap_end__,    (uint32_t*) &__RAM_segment_end__, "free")) return 0;
  return 1;
  #undef Test
}
/*---------------------------------------------------------------------------
  LED
  ---------------------------------------------------------------------------*/
static void HAL_SysLEDInit(void)
{
  PIOA->PIO_PER = BIT(BIT_SD_DET);  // PIO Enable Register */
  HAL_SysLED(true);
}
/*---------------------------------------------------------------------------
  SPI
  ---------------------------------------------------------------------------*/
static void HAL_SPIEthInit(void)
{
  //DEBUG("SPIInit\n");

  PIOA->PIO_PDR = BIT(BIT_MISO) | BIT(BIT_MOSI) | BIT(BIT_SCK);
  PIOA->PIO_MDDR = BIT(BIT_MISO) | BIT(BIT_MOSI) | BIT(BIT_SCK);

  PMC->PMC_PCER0 = BIT(ID_SPI); // SPI enable */

  // SPI write protection mode register */
  SPI->SPI_WPMR = 0x53504900; // Disable write protection */

  // SPI control register */
  SPI->SPI_CR = SPI_CR_SPIDIS; // Disable SPI */
  SPI->SPI_CR = SPI_CR_SWRST;  // Reset SPI */
  SPI->SPI_CR = SPI_CR_SWRST;  // Reset SPI */

  // SPI mode register */
  SPI->SPI_MR =
    SPI_MR_MSTR |   // Master mode */
    SPI_MR_MODFDIS; // Mode fault detection disabled */

  // SPI chip select register */
  SPI->SPI_CSR[0] =
    SPI_CSR_NCPHA |           // Mode 0: CPOL = 0,  NCPHA = 1 */
    SPI_CSR_CSAAT |           // Chip Select Active After Transfer */
    (0 << SPI_CSR_BITS_Pos) | // 8 bit */
    (12 << SPI_CSR_SCBR_Pos); // Baud rate MCK / 12 = 120MHz / 12 = 10MHz */

  SPI->SPI_CR = SPI_CR_SPIEN; // Enable SPI */

  // SPI write protection mode register */
  SPI->SPI_WPMR = 0x53504901; // Enable write protection */
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SPIEthWriteRead(uint8_t send)
{
  while ((SPI->SPI_SR & SPI_SR_TXEMPTY) == 0);
  SPI->SPI_TDR = send | (~(1 << SPI_TDR_PCS_Pos) & SPI_MR_PCS_Msk);
  while ((SPI->SPI_SR & SPI_SR_TDRE) == 0);
  while ((SPI->SPI_SR & SPI_SR_RDRF) == 0);
  uint8_t recv = SPI->SPI_RDR;
  return recv;
}
/*---------------------------------------------------------------------------
  ETH
  ---------------------------------------------------------------------------*/
void HAL_EthInit(void)
{
  PIOA->PIO_PER = BIT(BIT_ETH_CS); // PIO Enable Register */
  PIOA->PIO_OER = BIT(BIT_ETH_CS); // Output Enable Register */
  HAL_EthCS1();

  PIOB->PIO_PER = BIT(BIT_ETH_RES); // PIO Enable Register */
  PIOB->PIO_OER = BIT(BIT_ETH_RES); // Output Enable Register */
  HAL_EthReset();
}
/*---------------------------------------------------------------------------*/
void HAL_EthReset(void)
{
  uint16_t j = 0;
  PIOB->PIO_CODR = BIT(BIT_ETH_RES);
  for (uint8_t i = 0;i < 8;i++) while (++j) {}
  PIOB->PIO_SODR = BIT(BIT_ETH_RES);
  for (uint8_t i = 0;i < 16;i++) while (++j) {}
}
/*---------------------------------------------------------------------------*/
void HAL_EthCS0(void)
{
  PIOA->PIO_CODR = BIT(BIT_ETH_CS); // /CS = 0 */
}
/*---------------------------------------------------------------------------*/
void HAL_EthCS1(void)
{
  PIOA->PIO_SODR = BIT(BIT_ETH_CS); // /CS = 1 */
}
/*---------------------------------------------------------------------------
  ATSHA204
  ---------------------------------------------------------------------------*/
/*!
 * \brief Init ATSHA204
 */
#ifdef HAS_ATSHA204
  static void HAL_ATSHA204Init(void)
  {
    PIOA->PIO_PDR = BIT(BIT_CRYP_SDA_TX) | BIT(BIT_CRYP_SDA_RX);
    //PIOA->PIO_MDDR = BIT(BIT_CRYP_SDA_TX) | BIT(BIT_CRYP_SDA_RX);

    PMC->PMC_PCER0 = BIT(ID_USART0); // USART0 enable */

    USART0->US_WPMR = 0x55534100; // Disable write protection */

    USART0->US_IER = 0xFFFFFFFF; // Disable all interrupts */

    USART0->US_CR =
      US_CR_RSTTX | // Reset Receiver */
      US_CR_RSTRX;  // Reset Transmitter */

    USART0->US_CR =
      US_CR_RSTRX | // Reset Receiver */
      US_CR_RSTTX;  // Reset Transmitter */

    USART0->US_CR =
      US_CR_TXEN |  // The transmitter is enabled */
      US_CR_RXEN |  // The receiver is enabled */
      US_CR_RSTSTA; // Reset Status Bits */

    USART0->US_MR =
      (0 << US_MR_USCLKS_Pos) |  // Master Clock MCK is selected */
      (2 << US_MR_CHRL_Pos)   |  // 7bit */
      (4 << US_MR_PAR_Pos)    |  // No parity */
      (0 << US_MR_NBSTOP_Pos) |  // 1 stop bit */
      (0 << US_MR_CHMODE_Pos) |  // Normal Mode */
      US_MR_OVER;                // Oversampling Mode

    uint32_t cd = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 230400);
    USART0->US_BRGR = cd;

    PIOA->PIO_PUER = BIT(BIT_CRYP_SDA_TX);
    PIOA->PIO_PUER = BIT(BIT_CRYP_SDA_RX);

    //////////////USART0->US_WPMR = 0x55534101; // Enable write protection */
  }
#endif
/*---------------------------------------------------------------------------*/
#define SWI_FUNCTION_RETCODE_SUCCESS ((uint8_t) 0x00) // Communication with device succeeded
#define SWI_FUNCTION_RETCODE_TIMEOUT ((uint8_t) 0xF1) // Communication timed out
#define SWI_FUNCTION_RETCODE_RX_FAIL ((uint8_t) 0xF9) // Communication failed after at least one byte was received
/*---------------------------------------------------------------------------*/
uint8_t HAL_SHA204Send(uint8_t sendCount, uint8_t *send)
{
  PIOA->PIO_PDR = BIT(BIT_CRYP_SDA_TX); // Switch tx pin to USART,  drive output
  while (sendCount--) {
    USART0->US_RHR;
    uint8_t byte = *send++;
    for (uint8_t i = 0;i <= 7;i++) {
      while ((USART0->US_CSR & US_CSR_TXRDY) == 0);
      USART0->US_THR = byte & 1 ? 0x7F : 0x7D;
      byte >>= 1;
    }
  }
  while ((USART0->US_CSR & US_CSR_TXEMPTY) == 0);
  PIOA->PIO_PER = BIT(BIT_CRYP_SDA_TX); // Switch tx pin to PIO, input and pullup
  USART0->US_RHR;
  return SWI_FUNCTION_RETCODE_SUCCESS;
}
/*---------------------------------------------------------------------------*/
uint8_t HAL_SHA204Recv(uint8_t recvCount, uint8_t *recv)
{
  PIOA->PIO_PER = BIT(BIT_CRYP_SDA_TX); // Switch tx pin to PIO, input and pullup
  USART0->US_RHR;
  for (uint8_t i = 0; i < recvCount; i++) {
    uint8_t byte = 0;
    for (uint8_t j = 0;j <= 7;j++) {
      uint32_t timeout = (uint32_t) hal.delay.calibrate10 / 10 / 10; // 0.1ms
      while ((USART0->US_CSR & US_CSR_RXRDY) == 0)
        if (!timeout--)
          return i == 0 ? SWI_FUNCTION_RETCODE_TIMEOUT : SWI_FUNCTION_RETCODE_RX_FAIL;
      uint8_t pattern = USART0->US_RHR;
      // Analyse received pattern
      uint8_t mask = 0xFF; // Mask to detect zeros in pattern
      // If start bit is too long (one or two lowest bits can be zero) then adjust mask
      if (!(pattern & 1)) {
        mask <<= 1;
        if (!(pattern & 2)) mask <<= 1;
      }
      mask &= 0x7F; // 7 bit transfer,  so ignore bit 7
      // Buil the byte. If there are no zeros in the mask area we recived a 1
      byte = (byte >> 1) | (~pattern & mask ? 0x00 : 0x80); // Lowest bit first
    }
    *recv++ = byte;
  }
  return SWI_FUNCTION_RETCODE_SUCCESS;
}
/*---------------------------------------------------------------------------*/
void HAL_SHA204SignalPin(uint8_t high)
{
  PIOA->PIO_PDR = BIT(BIT_CRYP_SDA_TX); // Switch tx pin to USART
  if (high) {
    USART0->US_CR = US_CR_STPBRK; // Stop sending break, pin goes high
  } else {
    while ((USART0->US_CSR & US_CSR_TXRDY) == 0);
    USART0->US_CR = US_CR_STTBRK; // Start sending break, pin goes low
  }
}
/*---------------------------------------------------------------------------*/
uint32_t clock(void)
{
  return hal.tickMS;
}
/*---------------------------------------------------------------------------*/
int putchar(int __c)
{
  #ifdef REDIRECT_STDOUT_TO_RS232
    // Output to RS232
    HAL_RS232Tx(__c);
  #endif
  #ifdef REDIRECT_STDOUT_TO_DEBUG
    // Output to JTAG debug interface
    {
      static char buffer[80];
      static uint8_t bufferP = 0;
      buffer[bufferP++] = (char) __c;
      if ((__c == 10) || (bufferP >= sizeof(buffer) - 1) || !putcharFlushTimer) {
        buffer[bufferP] = 0;
        DEBUG("%s", buffer);
        bufferP = 0;
        putcharFlushTimer = 100;
      }
    }
  #endif
  #ifdef REDIRECT_STDOUT_TO_SYSLOG
    {
      static char buffer[200];
      static uint8_t bufferP = 0;
      if (bufferP < sizeof(buffer) - 1)
        buffer[bufferP++] = (char) __c;
      if (__c == 10) {
        buffer[bufferP] = 0;
        SysLog_Send(SYSLOG_PRINTF | SYSLOG_INFO, buffer);
        bufferP = 0;
      }
    }
  #endif
  return __c;
}
/*---------------------------------------------------------------------------
  Pin
  ---------------------------------------------------------------------------*/
Pio* HAL_GetPinPio(int16_t pinId)
{
  return pinId & BIT(5) ? PIOB : PIOA;
}
/*---------------------------------------------------------------------------*/
uint32_t HAL_GetPinMask(int16_t pinId)
{
  return BIT(pinId & 0x1F);
}
/*---------------------------------------------------------------------------*/
const char* HAL_PinGetNames()
{
  return
    "PA0,PA1,PA2,PA3,PA4,PA7,PA8,PA9,PA10,PA15,PA16,PA17,PA18,PA19,PA20,PA21,PA22,PA23,PA24,"
    "PB0,PB1,PB2,PB3,PB10,PB11,PB12";
}
/*---------------------------------------------------------------------------*/
int16_t HAL_PinNameToId(const char *name)
{
  if ((strlen(name) <= 3) && isdigit(name[1])) {
    int pin = atoi(&name[1]);
    if ((pin >= 0) && (pin <= 31) && (name[0] >= 'A') && (name[0] <= 'B')) {
      int16_t pinId = PINID(name[0], pin);
      return ResourceIdToIndex(pinId) >= 0 ? pinId : RESULT_HAL_ERROR_UNKNOWN_PIN_NAME;
    }
  }
  return RESULT_HAL_ERROR_UNKNOWN_PIN_NAME;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_PinConfig(int16_t pinId, const char *config, uint8_t configNumber)
{
  bool Match(const char *text)
  {
    int length = strlen(config);
    return length && (strncmp(config, text, length) == 0);
  }

  Pio *pio = HAL_GetPinPio(pinId);
  uint32_t pinMask = HAL_GetPinMask(pinId);
  if (Match("FREE")) {
    pio->PIO_PDR = pinMask; // PIO Disable Register
    ResourceUnlock(RESOURCE_OWNER_PIN);
  } else {
    if (!configNumber) {
      int16_t result = ResourceLock1(RESOURCE_OWNER_PIN, pinId);
      if (result < 0) 
        return result;
    }
    if (Match("RESET")) {
      pio->PIO_ODR = pinMask;   // PIO Output Disable Register
      pio->PIO_PUDR = pinMask;  // Pull-up Disable Register
      pio->PIO_PPDDR = pinMask; // Pad Pull-down Disable Register
    } else if (Match("INPUT"))
      pio->PIO_ODR = pinMask; // PIO Output Disable Register
    else if (Match("OUTPUT"))
      pio->PIO_OER = pinMask; // PIO Output Enable Register
    else if (Match("LOW")) {
      pio->PIO_CODR = pinMask; // PIO Clear Output Data Register
      pio->PIO_OER = pinMask;  // PIO Output Enable Register
    } else if (Match("HIGH")) {
      pio->PIO_SODR = pinMask; // PIO Set Output Data Register
      pio->PIO_OER = pinMask;  // PIO Output Enable Register
    } else if (Match("PULLUP")) {
      pio->PIO_PUER = pinMask;
      pio->PIO_PPDDR = pinMask;
    } else if (Match("PULLDOWN")) {
      pio->PIO_PUDR = pinMask;
      pio->PIO_PPDER = pinMask;
    } else
      return RESULT_HAL_ERROR_WRONG_PARAMETER;

    pio->PIO_PER = pinMask; // PIO Enable Register
  }
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HAL_PinSet(int16_t pinId, bool high)
{
  //Pio *pio = (id >> 5) & 1 ? PIOB : PIOA;
  Pio *pio = HAL_GetPinPio(pinId);
  uint32_t pinMask = HAL_GetPinMask(pinId);
  if (high)
    pio->PIO_SODR = pinMask; // PIO Set Output Data Register
  else
    pio->PIO_CODR = pinMask; // PIO Clear Output Data Register
}
/*---------------------------------------------------------------------------*/
bool HAL_PinGet(int16_t pinId)
{
  Pio *pio = HAL_GetPinPio(pinId);
  uint32_t pinMask = HAL_GetPinMask(pinId);
  return (pio->PIO_PDSR & pinMask) > 0; // PIO Pin Data Status Register
}
/*---------------------------------------------------------------------------
  Serial
  ---------------------------------------------------------------------------*/
void HAL_SerialInit(void)
{
  static uint8_t buffer0A[1 + 512]; // Size for DMX
  static uint8_t buffer0B[1 + 512]; // Size for DMX
  static uint8_t buffer1[64];
  static uint8_t buffer2[64];
  memset(serial, 0, sizeof(serial));
  serial[0].recv.a.buffer = buffer0A;
  serial[0].recv.b.buffer = buffer0B;
  serial[0].recv.size = sizeof(buffer0A);
  serial[1].recv.a.buffer = buffer1;
  serial[1].recv.size = sizeof(buffer1);
  serial[2].recv.a.buffer = buffer2;
  serial[2].recv.size = sizeof(buffer2);
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SerialOpen(uint16_t channel)
{
  void OpenUSART(Usart *usart)
  {
    usart->US_WPMR = 0x55534100; // Disable write protection 
    usart->US_IER = 0xFFFFFFFF;  // Disable all interrupts 
    usart->US_CR =
      US_CR_RSTTX | // Reset Receiver 
      US_CR_RSTRX;  // Reset Transmitter 
    usart->US_CR =
      US_CR_RSTRX | // Reset Receiver 
      US_CR_RSTTX;  // Reset Transmitter */
    usart->US_CR =
      US_CR_TXEN |  // The transmitter is enabled 
      US_CR_RXEN |  // The receiver is enabled 
      US_CR_RSTSTA; // Reset Status Bits 
    usart->US_MR =
      (0 << US_MR_USCLKS_Pos) |  // Master Clock MCK is selected 
      (3 << US_MR_CHRL_Pos)   |  // 8 bit 
      (4 << US_MR_PAR_Pos)    |  // No parity 
      (0 << US_MR_NBSTOP_Pos) |  // 1 stop bit 
      (0 << US_MR_CHMODE_Pos) |  // Normal Mode 
      US_MR_OVER;                // Oversampling Mode
    usart->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 19200);
    usart->US_WPMR = 0x55534101; // Enable write protection
    usart->US_IDR = 0xFFFFFFFF;
    usart->US_IER =
      US_IER_RXRDY |  // RXRDY Interrupt Enable 
      US_IER_RXBRK; // |  // Receiver Break Interrupt Enable 
      //US_IER_OVRE  |  // Overrun Error Interrupt Enable 
      //US_IER_FRAME |  // Framing Error Interrupt Enable 
      //US_IER_PARE;    // Parity Error Interrupt Enable 
  }

  void OpenUART(Uart *uart)
  {
    uart->UART_IDR = 0xFFFFFFFF; // Disable all interrupts 
    uart->UART_CR =
      UART_CR_RSTRX | // Reset Receiver 
      UART_CR_RSTTX;  // Reset Transmitter 
    uart->UART_CR =
      UART_CR_RSTRX | // Reset Receiver 
      UART_CR_RSTTX;  // Reset Transmitter 
    uart->UART_CR =
      UART_CR_RXEN |  // The receiver is enabled 
      UART_CR_TXEN |  // The transmitter is enabled 
      UART_CR_RSTSTA; // Reset Status Bits 
    uart->UART_MR =
      (4 << UART_MR_PAR_Pos) |    // No parity 
      (0 << UART_MR_CHMODE_Pos);  // Normal Mode 
    uart->UART_BRGR = SystemCoreClock / ((uint32_t) 16 * (uint32_t) 19200);
    uart->UART_IER =
      UART_IER_RXRDY |  // RXRDY Interrupt Enable 
      UART_IER_OVRE  |  // Overrun Error Interrupt Enable 
      UART_IER_FRAME |  // Framing Error Interrupt Enable 
      UART_IER_PARE;    // Parity Error Interrupt Enable 
  }

  Serial *s = &serial[channel];
  switch (channel) {

    case 0 :
      {
        int16_t result = ResourceLock2(RESOURCE_OWNER_SERIAL_0, PINID('A', 21), PINID('A', 23));
        if (result < 0) 
          return result;
      }
      PMC->PMC_PCER0 = BIT(ID_USART1);   // USART1 enable 
      OpenUSART(USART1);
      PIOA->PIO_PDR = BIT(21) | BIT(22); // Switch rx and tx pin to USART
      NVIC->ISER[0] = BIT(USART1_IRQn);  // Enable USART1 interrupts 
      s->modes |= SERIAL_MODE_ISOPEN;
      return RESULT_OK;

    case 1 :
      {
        int16_t result = ResourceLock2(RESOURCE_OWNER_SERIAL_1, PINID('A', 9), PINID('A', 10));
        if (result < 0) 
          return result;
      }
      PMC->PMC_PCER0 = BIT(ID_UART0);   // UART0 enable */
      OpenUART(UART0);
      PIOA->PIO_PDR = BIT(9) | BIT(10); // Switch rx and tx pin to UART
      NVIC->ISER[0] = BIT(UART0_IRQn);  // Enable UART0 interrupts 
      s->modes |= SERIAL_MODE_ISOPEN;
      return RESULT_OK;

    case 2 :
      {
        int16_t result = ResourceLock2(RESOURCE_OWNER_SERIAL_2, PINID('B', 2), PINID('B', 3));
        if (result < 0) 
          return result;
      }
      PMC->PMC_PCER0 = BIT(ID_UART1);  // UART1 enable 
      OpenUART(UART1);
      PIOB->PIO_PDR = BIT(2) | BIT(3); // Switch rx and tx pin to UART
      NVIC->ISER[0] = BIT(UART1_IRQn); // Enable UART1 interrupts 
      s->modes |= SERIAL_MODE_ISOPEN;
      return RESULT_OK;

  }
  return RESULT_HAL_ERROR_WRONG_CHANNEL;
}
/*---------------------------------------------------------------------------*/
void HAL_SerialClose(uint16_t channel)
{
  switch (channel) {
    case 0 :
      PMC->PMC_PCDR0 = BIT(ID_USART1); // USART1 disable 
      ResourceUnlock(RESOURCE_OWNER_SERIAL_0);
      break;
    case 1 :
      PMC->PMC_PCDR0 = BIT(ID_UART0); // UART0 disable 
      ResourceUnlock(RESOURCE_OWNER_SERIAL_1);
      break;
    case 2 :
      PMC->PMC_PCDR0 = BIT(ID_UART1); // UART1 disable 
      ResourceUnlock(RESOURCE_OWNER_SERIAL_2);
      break;
    default :
      return;
  }
  Serial *s = &serial[channel];
  s->modes = 0;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SerialConfig(uint16_t channel, const char *config, uint8_t configNumber)
{
  Usart *usart = 0;
  Uart *uart = 0;

  bool Match(const char *text)
  {
    int length = strlen(config);
    return length && (strncmp(config, text, length) == 0);
  }

  int16_t Config(void)
  {
    Serial *s = &serial[channel];
    if (!(s->modes & SERIAL_MODE_ISOPEN)) 
      return RESULT_HAL_ERROR_IS_NOT_OPEN;
    s->modes &= ~SERIAL_MODE_ISDMX;
    int value = atoi(config);
    if (value >= 1) {
      if (value <= 2) {
        // Stop bits 1..2
        Modifyer_32AutoShift(&usart->US_MR, US_MR_NBSTOP_Msk, value & ~1);
        if (uart) 
         return RESULT_HAL_ERROR_WRONG_PARAMETER;
      } else if ((value >= 5) && (value <= 8)) {
        // Data bits 5..8
        Modifyer_32AutoShift(&usart->US_MR, US_MR_CHRL_Msk, value - 5);
        if (uart) 
          return RESULT_HAL_ERROR_WRONG_PARAMETER;
      } else if (value >= 100) {
        // Baudrate >= 100
        if (usart) 
          usart->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) value);
        if (uart) 
          uart->UART_BRGR = SystemCoreClock / ((uint32_t) 16 * (uint32_t) value);
      } else
        return RESULT_HAL_ERROR_WRONG_PARAMETER;
    } else if (Match("DMX")) { 
      if (channel != 0)
        return RESULT_HAL_ERROR_WRONG_CHANNEL;
      s->modes |= SERIAL_MODE_ISDMX;
      usart->US_CR = 0;
      usart->US_CR = US_CR_TXEN;     
      usart->US_MR =
      US_MR_USCLKS_MCK |       // Master Clock MCK is selected 
      US_MR_CHRL_8_BIT |       // 8 bit 
      US_MR_PAR_NO |           // No parity 
      US_MR_NBSTOP_2_BIT |     // 2 stop bits 
      US_MR_USART_MODE_RS485 | // RS485 Mode 
      US_MR_OVER;              // Oversampling Mode
      usart->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 250000L); // Set DMX baud rate
      return RESULT_OK; // Return here, no other configs make sense!
    }
    s->modes &= ~SERIAL_MODE_ISDMX;
    int8_t result = RESULT_HAL_ERROR_WRONG_PARAMETER;
    static const char* modeNames[] = {"RS232", "RS485", "", "", "", "IS07816-T0", "IS07816-T1", "IRDA", 0};
    if (usart) 
      result = Modifyer_32ByName(modeNames, &usart->US_MR, US_MR_USART_MODE_Msk, 0, config);
    // TODO TXEN resource!
    if (result < 0) 
      return result;
    static const char* parityNames[] = {"EVEN", "ODD", "SPACE", "MARK", "NOPARITY", 0};
    if (usart) 
      result = Modifyer_32ByName(parityNames, &usart->US_MR, US_MR_PAR_Msk, 0, config);
    if (uart) 
      result = Modifyer_32ByName(parityNames, &uart->UART_MR, UART_MR_CHMODE_Msk, 0, config);
    return result;
  }

  switch (channel) {
    case 0 :
      usart = USART1;
      break;
    case 1 :
      uart = UART0;
      break;
    case 2 :
      uart = UART1;
      break;
    default :
      return -1;
  }
  if (usart) 
    usart->US_WPMR = 0x55534100; // Disable write protection 
  int16_t result = Config();
  if (usart) 
    usart->US_WPMR = 0x55534101; // Enable write protection 
  return result;
}
/*---------------------------------------------------------------------------*/
int32_t HAL_SerialSend(uint16_t channel, const uint8_t *send, uint32_t size)
{
  int32_t Send(const uint8_t *send, uint32_t size)
  {
    for (uint32_t i = 0; i < size; i++) 
      switch (channel) {
        case 0 :
          while ((USART1->US_CSR & US_CSR_TXRDY) == 0);
          USART1->US_THR = *send++;;
          break;
        case 1 :
          while ((UART0->UART_SR & UART_SR_TXRDY) == 0);
          UART0->UART_THR = *send++;
          break;
        case 2 :
          while ((UART1->UART_SR & UART_SR_TXRDY) == 0);
          UART1->UART_THR = *send++;
          break;
      }
    return size;
  }
  if (channel >= 3) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  Serial *s = &serial[channel];
  if (!(s->modes & SERIAL_MODE_ISOPEN))  
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  if (s->modes & SERIAL_MODE_ISDMX) {
    if (size > 1 + 512)
      return RESULT_HAL_ERROR_WRONG_PARAMETER;    
    if (channel != 0)
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    USART1->US_WPMR = 0x55534100;                                            // Disable write protection 
    USART1->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 50000L);  // Duration of one bit is now 20µs
    uint8_t temp = 0xC0;                                                     // 7 low = 140µs, 3 high = 60µs
    int32_t result = Send(&temp, 1);                                         // Break + Mark before startbyte 
    if (result < 0)
      return result;
    while ((USART1->US_CSR & US_CSR_TXEMPTY) == 0);                          // Wait on Data transmit end
    USART1->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 250000L); // Back to DMX baud rate
    USART1->US_WPMR = 0x55534101;                                            // Enable write protection 
    // First byte must be in the buffer. Then it is possible to send 
    // other start byte values than zero
    //temp = 0;
    //result = Send(&temp, 1); // Startbyte = 0
    //if (result < 0)
    //  return result;
  }
  return Send(send, size);
}
/*---------------------------------------------------------------------------*/
int32_t HAL_SerialRecv(uint16_t channel, uint8_t *recv, uint32_t size)
{
  if (channel >= 3) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  Serial *s = &serial[channel];
  if (!(s->modes & SERIAL_MODE_ISOPEN))  
    return RESULT_HAL_ERROR_IS_NOT_OPEN;
  if (s->modes & SERIAL_MODE_ISDMX) {
    if (!recv)                                         // Request count of chars
      return s->recv.b.top;                            // so return the number of bytes
    GlobalInterruptsDisable();
    if (size > s->recv.b.top) 
      size = s->recv.b.top;
    memcpy(recv, (uint8_t*) s->recv.b.buffer, size);
    s->recv.b.top = 0;                                 // Free the secondary buffer
    GlobalInterruptsEnable();
    return size;
  }
  if (!recv) // Request for count of chars
    return s->recv.a.top;
  GlobalInterruptsDisable();
  if (size > s->recv.a.top) 
    size = s->recv.a.top;
  memcpy(recv, (uint8_t*) s->recv.a.buffer, size);
  memmove((uint8_t*) &s->recv.a.buffer[0], (uint8_t*) &s->recv.a.buffer[size], s->recv.a.top - size);
  s->recv.a.top -= size;
  GlobalInterruptsEnable();
  return size;
}
/*---------------------------------------------------------------------------*/
static void SerialPushRecv(Serial *s, uint8_t byte)
{
  if (s->recv.a.top < s->recv.size) 
    s->recv.a.buffer[s->recv.a.top++] = byte;
}
/*---------------------------------------------------------------------------*/
/*
  static bool toggle;
  if (toggle)
    PIOA->PIO_SODR = BIT(0); // Set output Data Register
  else
    PIOA->PIO_CODR = BIT(0); // Clear output Data Register
  toggle = !toggle;
*/
void USART1_Handler(void)
{
  //PIOA->PIO_CODR = BIT(0); // Clear output Data Register
  Serial *s = &serial[0];
  uint32_t status = USART1->US_CSR;
  uint8_t byte = USART1->US_RHR;
  USART1->US_CR = US_CR_RSTSTA; // Reset Status Bits
  if (s->modes & SERIAL_MODE_ISDMX) {
    // DMX mode
    if (status & US_CSR_RXBRK) {                                     // Break received
      if (s->recv.a.top && !s->recv.b.top) {                         // If I have reveived data and the second buffer is free
        if (!(s->modes & SERIAL_MODE_ERROR)) {
          memcpy(s->recv.b.buffer, s->recv.a.buffer, s->recv.a.top); // copy primary to second buffer 
          s->recv.b.top = s->recv.a.top;                             // Set the count of bytes
        }
        s->recv.a.top = 0;                                           // Make primary buffer free
        s->timeoutMS = 1000;                                         // DMX interdigit and frame timeout is 1 second
        s->modes &= ~SERIAL_MODE_ERROR;                              // Reset error flag
      }
    } else if (status & US_CSR_RXRDY) {                              // I have received a byte
      SerialPushRecv(s, byte);                                       // so push it to the primary buffer
      s->timeoutMS = 1000;                                           // DMX interdigit and frame timeout is 1 second
    } else { //if (status & US_CSR_FRAME) {
      s->modes |= SERIAL_MODE_ERROR;                                 // Set error flag
    }
  } else {
    // Normal mode
    if (status & US_CSR_RXBRK)
      SerialPushRecv(s, 0);
    else if (status & US_CSR_RXRDY)
      SerialPushRecv(s, byte);
  }
  //PIOA->PIO_SODR = BIT(0); // Set output Data Register
}
/*---------------------------------------------------------------------------*/
void UART0_Handler(void)
{
  Serial *s = &serial[1];
  uint32_t status = UART0->UART_SR;
  uint8_t byte = UART0->UART_RHR;
  UART0->UART_CR = UART_CR_RSTSTA; // Reset Status Bits 
  if (status & UART_SR_RXRDY)
    SerialPushRecv(s, byte);
}
/*---------------------------------------------------------------------------*/
void UART1_Handler(void)
{
  Serial *s = &serial[2];
  uint32_t status = UART1->UART_SR;
  uint8_t byte = UART1->UART_RHR;
  UART1->UART_CR = UART_CR_RSTSTA; // Reset Status Bits 
  if (status & UART_SR_RXRDY)
    SerialPushRecv(s, byte);
}
/*---------------------------------------------------------------------------
  SPI
  ---------------------------------------------------------------------------*/
int16_t HAL_SPIOpen(uint16_t channel)
{
  int16_t result = ResourceLock2(RESOURCE_OWNER_SPI, PINID('A', 21), PINID('A', 23));
  if (result < 0) 
    return result;
  USART1->US_WPMR = 0x55534100; // Disable write protection 
  USART1->US_IER = 0xFFFFFFFF;  // Disable all interrupts 
  USART1->US_CR =
    US_CR_RSTTX | // Reset Receiver */
    US_CR_RSTRX;  // Reset Transmitter */
  USART1->US_CR =
    US_CR_RSTRX | // Reset Receiver */
    US_CR_RSTTX;  // Reset Transmitter */
  USART1->US_CR =
    US_CR_TXEN |  // The transmitter is enabled */
    US_CR_RXEN |  // The receiver is enabled */
    US_CR_RSTSTA; // Reset Status Bits */
  USART1->US_MR =
    (0 << US_MR_USCLKS_Pos) |    // Master Clock MCK is selected */
    (3 << US_MR_CHRL_Pos)   |    // 8 bit */
    (14 << US_MR_USART_MODE_Pos); // SPI master */
  USART1->US_BRGR = SystemCoreClock / ((uint32_t) 8 * (uint32_t) 19200);
  USART1->US_WPMR = 0x55534101; // Enable write protection */
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HAL_SPIClose(uint16_t channel)
{
  ResourceUnlock(RESOURCE_OWNER_SPI);
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SPIConfig(uint16_t channel, const char *config, uint8_t configNumber)
{
  if (channel >= 1) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  // TODO
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SPISendRecvByte(uint16_t channel, uint8_t send)
{
  if (channel >= 1) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  // TODO
  uint8_t buffer = send;
  int16_t result = HAL_SPISendRecv(channel, &buffer, 1);
  if (result < 0) 
    return result;
  return buffer;
}
/*---------------------------------------------------------------------------*/
int32_t HAL_SPISendRecv(uint16_t channel, uint8_t *buffer, uint32_t size)
{
  if (channel >= 1) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  for (uint32_t i = 0;i < size; i++) {
    int16_t result = HAL_SPISendRecvByte(channel, *buffer);
    if (result < 0) 
      return result;
    *buffer++ = result;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
int32_t HAL_SPISend(uint16_t channel, const uint8_t *send, uint32_t size)
{
  if (channel >= 1) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  for (uint32_t i = 0;i < size; i++) {
    int16_t result = HAL_SPISendRecvByte(channel, *send++);
    if (result < 0) 
      return result;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
int32_t HAL_SPIRecv(uint16_t channel, uint8_t *recv, uint32_t size)
{
  if (channel >= 1) 
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  for (uint32_t i = 0;i < size; i++) {
    int16_t result = HAL_SPISendRecvByte(channel, 0xFF);
    if (result < 0) 
      return result;
    *recv++ = result;
  }
  return size;
}
/*---------------------------------------------------------------------------
  I2C
  ---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  static int16_t I2CCalcBaudrate(uint32_t baudRate)
  {
    // Calc baud rate registers
    // B = F / (A * 2^B + 4)
    // F/B = A * 2^B + 4
    // F/B - 4 = A * 2^B
    uint32_t fb4 = SystemCoreClock / baudRate - 4;
    uint8_t div = 0;
    while (fb4 & 0xFFFFFE00) div++, fb4 >>= 1;
    if (div > 7)
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    fb4 >>= 1;
    i2c.CWGR =
      TWI_CWGR_CLDIV(fb4) |
      TWI_CWGR_CHDIV(fb4) |
      TWI_CWGR_CKDIV(div); 
    return RESULT_OK;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  static void I2CReset(void)
  {
    TWI0->TWI_CR =
      TWI_CR_SWRST;                      // Software Reset
    TWI0->TWI_CWGR = i2c.CWGR;
    TWI0->TWI_CR =
      TWI_CR_MSDIS |                     // TWI Master Mode Disable
      TWI_CR_SVDIS;                      // TWI Slave Mode Disabled
    TWI0->TWI_CR =
      TWI_CR_MSEN;                       // TWI Master Mode Enabled
    (void) TWI0->TWI_SR;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  int16_t HAL_I2COpen(uint16_t channel)
  {
    i2c.timeoutUS = 5000L; // 5ms
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    int16_t result = ResourceLock2(RESOURCE_OWNER_I2C, PINID('A', 3), PINID('A', 4));
    if (result < 0) 
      return result;
    PMC->PMC_PCER0 = BIT(ID_TWI0);       // TWI enable 
    PIOA->PIO_PDR = BIT(3) | BIT(4);     // Switch pins to TWI
    PIOA->PIO_MDDR = BIT(3) | BIT(4);
    TWI0->TWI_IDR = 0xFFFFFFFF;          // Disable all interrupts
    result = I2CCalcBaudrate(100000L);   // Set rate to 100k, default
    if (result < 0)
      return result;
    I2CReset();
    i2c.isOpen = true;
    return RESULT_OK;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  void HAL_I2CClose(uint16_t channel)
  {
    PMC->PMC_PCDR0 = BIT(ID_TWI0); // TWI disable
    i2c.isOpen = false;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  int16_t HAL_I2CConfig(uint16_t channel, const char *config)
  {
    if (!i2c.isOpen)
      return RESULT_HAL_ERROR_IS_NOT_OPEN;
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    int value = atoi(config);
    if ((value >= 100) || (value <= 1000000)) {
      return I2CCalcBaudrate(value);
    } else if (memcmp(config, "TIMEOUT", 7) == 0) { 
      int32_t timeoutUS = atof(config[7]) * 1.0E6; 
      if ((timeoutUS <= 0) || (timeoutUS > 1000000L))
        return RESULT_HAL_ERROR_WRONG_PARAMETER;
      i2c.timeoutUS = timeoutUS;
    } else
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    return RESULT_OK;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  int16_t HAL_I2CExist(uint16_t channel, uint8_t addr)
  {
    if (!i2c.isOpen)
      return RESULT_HAL_ERROR_IS_NOT_OPEN;
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    if (addr > 127)
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    I2CReset();
    TWI0->TWI_MMR =
      TWI_MMR_DADR(addr) |
      TWI_MMR_MREAD;
    TWI0->TWI_CR =
      TWI_CR_START | 
      TWI_CR_STOP;  
    uint32_t top = i2c.timeoutUS * hal.delay.calibrate10 / 10000;
    uint32_t status;
    DELAY_US_BREAK(top, (status = TWI0->TWI_SR) & TWI_SR_TXCOMP);   
    return status & TWI_SR_NACK ? RESULT_ERROR_TIMEOUT : RESULT_OK;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  int16_t HAL_I2CSend(uint16_t channel, uint8_t addr, uint32_t internalAddr, uint8_t internalAddrSize, uint8_t *send, uint16_t size)
  {
    if (!i2c.isOpen)
      return RESULT_HAL_ERROR_IS_NOT_OPEN;
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    if ((addr > 127) || (internalAddrSize > 3))
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    I2CReset();
    TWI0->TWI_MMR = 0;
    TWI0->TWI_MMR =
      TWI_MMR_DADR(addr) |
      (internalAddrSize << TWI_MMR_IADRSZ_Pos);
    TWI0->TWI_IADR = 0;
    TWI0->TWI_IADR = internalAddr;
    while (size--) {
      TWI0->TWI_THR = *send++;
      while ((TWI0->TWI_SR & TWI_SR_TXRDY) == 0);
    }
    TWI0->TWI_CR = TWI_CR_STOP;
    uint32_t top = i2c.timeoutUS * hal.delay.calibrate10 / 10000;
    uint32_t status;
    DELAY_US_BREAK(top, (status = TWI0->TWI_SR) & TWI_SR_TXCOMP);    
    return status & TWI_SR_TXCOMP ? RESULT_OK : RESULT_ERROR_TIMEOUT;     
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_I2C
  int16_t HAL_I2CRecv(uint16_t channel, uint8_t addr, uint32_t internalAddr, uint8_t internalAddrSize, uint8_t *recv, uint16_t size)
  {
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    if ((addr > 127) || (internalAddrSize > 3))
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    I2CReset();
    TWI0->TWI_MMR = 0;
    TWI0->TWI_MMR = 
      TWI_MMR_DADR(addr) |
      TWI_MMR_MREAD |
      (internalAddrSize << TWI_MMR_IADRSZ_Pos);
    TWI0->TWI_IADR = 0;
    TWI0->TWI_IADR = internalAddr;
    TWI0->TWI_CR = TWI_CR_START;
    while (size) {
      uint32_t status = TWI0->TWI_SR;
      if (status & TWI_SR_NACK) 
        return RESULT_ERROR_NO_ACK;
      if (size == 1)
        TWI0->TWI_CR = TWI_CR_STOP;
      if (!(TWI0->TWI_SR & TWI_SR_RXRDY))
        continue;
      *recv++ = TWI0->TWI_RHR; 
      size--;
    }
    uint32_t top = i2c.timeoutUS * hal.delay.calibrate10 / 10000;
    uint32_t status;
    DELAY_US_BREAK(top, (status = TWI0->TWI_SR) & TWI_SR_TXCOMP);    
    return status & TWI_SR_TXCOMP ? RESULT_OK : RESULT_ERROR_TIMEOUT;
  }
#endif
/*---------------------------------------------------------------------------*/
/*!
 * \brief Temperature sensor init.
 * \note The temperture sensor is on the CPU chip. The measued temperatur is
 * normally 20°C higher as the ambient temeperture
 */
#if 0 
  void HAL_TemperatureSensorInit(void)
  {
    PMC->PMC_PCER0 = BIT(ID_ADC);   // ADC enable 
    ADC->ADC_WPMR = 0x41444300;     // Disables the Write Protect 
    ADC->ADC_CR = ADC_CR_SWRST;     // Reset the ADC 
    //ADC->ADC_CR = ADC_CR_AUTOCAL;
    ADC->ADC_MR =
      (8 << ADC_MR_STARTUP_Pos) |   // 512 periods of ADCClock 
      (2 << ADC_MR_SETTLING_Pos) |  // Analog Settling Time 9 periods of ADCClock 
      (15 << ADC_MR_TRACKTIM_Pos) | // Tracking Time = (TRACKTIM + 1) * ADCClock periods  
      (2 << ADC_MR_TRANSFER_Pos);
    ADC->ADC_EMR = ADC_EMR_TAG;
    ADC->ADC_CGR = 0;
    ADC->ADC_COR = 0;
    ADC->ADC_ACR = ADC_ACR_TSON;    // Temperature sensor on 
    ADC->ADC_CHER = 0xFFFF;         // Use channel 0..15 
    ADC->ADC_CR = ADC_CR_START;     // Start the ADC 
    ADC->ADC_WPMR = 0x41444301;     // Enables the Write Protect 
  }
#endif
/*---------------------------------------------------------------------------
  LIS
  ---------------------------------------------------------------------------*/
#ifdef HAS_LIS
  int16_t HAL_LISOpen(void)
  {
    int16_t result = ResourceLock2(RESOURCE_OWNER_LIS, PINID('A', 21), PINID('A', 24));
    if (result < 0) return result;

    PMC->PMC_PCER0 = BIT(ID_USART1); // USART1 enable */

    PIOA->PIO_PDR = BIT(21) | BIT(22) | BIT(23); // Switch pins to USART1
    PIOA->PIO_ODR = BIT(21) | BIT(22) | BIT(23);
    PIOA->PIO_MDDR = BIT(21) | BIT(22) | BIT(23);

    PIOA->PIO_OER = BIT(24);  // Output Enable Register */
    PIOA->PIO_PER = BIT(24);
    PIOA->PIO_CODR = BIT(24); // Clear output Data Register */

    PIOA->PIO_PUER = BIT(21);

    USART1->US_WPMR = 0x55534100; // Disable write protection */
    USART1->US_IDR = 0xFFFFFFFF; // Disable all interrupts */
    USART1->US_CR =
      US_CR_RSTTX | // Reset Receiver */
      US_CR_RSTRX;  // Reset Transmitter */
    USART1->US_CR =
      US_CR_RSTRX | // Reset Receiver */
      US_CR_RSTTX;  // Reset Transmitter */
    USART1->US_CR =
      US_CR_TXEN |  // The transmitter is enabled */
      US_CR_RXEN |  // The receiver is enabled */
      US_CR_RSTSTA; // Reset Status Bits */
    USART1->US_MR =
      US_MR_USART_MODE_SPI_MASTER |
      US_MR_USCLKS_MCK |
      US_MR_CHRL_8_BIT |
      US_MR_CHMODE_NORMAL |
      US_MR_CPOL |
      US_MR_CLKO;

    uint32_t t = SystemCoreClock / (uint32_t) 4E6; // 4MHz
    USART1->US_BRGR = t & ~1;
    USART1->US_WPMR = 0x55534101; // Enable write protection */
    return 0;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_LIS
  void HAL_LISClose()
  {
    PMC->PMC_PCDR0 = BIT(ID_USART1); // USART1 disable */
    ResourceUnlock(RESOURCE_OWNER_LIS);
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_LIS
  void HAL_LISReset(void)
  {
  /*
    uint16 i = 0;
    PIOA_SODR = BIT(BIT_APWR);
    while (++i) {}
    PIOA_CODR = BIT(BIT_APWR);
    while (++i) {}
  */
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_LIS
  int16_t HAL_LISWrite16Read8(uint8_t send1, uint8_t send2)
  {
    (void) USART1->US_RHR;                          // Dummy read
    PIOA->PIO_CODR = BIT(24);                       // CS = 0
    USART1->US_THR = send1;                         // Send byte 1
    while ((USART1->US_CSR & US_CSR_TXRDY) == 0);   // Wait for complete
    USART1->US_THR = send2;                         // Send byte 2
    while ((USART1->US_CSR & US_CSR_TXEMPTY) == 0);
    PIOA->PIO_SODR = BIT(24);                       // CS = 1
    return USART1->US_RHR;
  }
#endif
/*---------------------------------------------------------------------------
  EVE
  ---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  int16_t HAL_EVEOpen(uint16_t channel)
  {
    if (channel) 
      return RESULT_HAL_ERROR_WRONG_CHANNEL;
    int16_t result = ResourceLock2(RESOURCE_OWNER_EVE, PINID('A', 21), PINID('A', 24));
    if (result < 0) 
      return result;

    PMC->PMC_PCER0 = BIT(ID_USART1); // USART1 enable */

    PIOA->PIO_PDR = BIT(21) | BIT(22) | BIT(23); // Switch pins to USART1
    PIOA->PIO_ODR = BIT(21) | BIT(22) | BIT(23);
    PIOA->PIO_MDDR = BIT(21) | BIT(22) | BIT(23);

    PIOA->PIO_OER = BIT(24);  // Output Enable Register */
    PIOA->PIO_PER = BIT(24);
    PIOA->PIO_CODR = BIT(24); // Clear output Data Register */

    PIOA->PIO_PUER = BIT(21);

    USART1->US_WPMR = 0x55534100; // Disable write protection */
    USART1->US_IDR = 0xFFFFFFFF; // Disable all interrupts */
    USART1->US_CR =
      US_CR_RSTTX | // Reset Receiver */
      US_CR_RSTRX;  // Reset Transmitter */
    USART1->US_CR =
      US_CR_RSTRX | // Reset Receiver */
      US_CR_RSTTX;  // Reset Transmitter */
    USART1->US_CR =
      US_CR_TXEN |  // The transmitter is enabled */
      US_CR_RXEN |  // The receiver is enabled */
      US_CR_RSTSTA; // Reset Status Bits */
    USART1->US_MR =
      US_MR_USART_MODE_SPI_MASTER |
      US_MR_USCLKS_MCK |
      US_MR_CHRL_8_BIT |
      US_MR_CHMODE_NORMAL |
      //US_MR_CPOL |
      US_MR_CPHA |  // Mode 0
      US_MR_CLKO;

    uint32_t t = SystemCoreClock / (uint32_t) 10E6; // 10MHz
    USART1->US_BRGR = t & ~1;
    USART1->US_WPMR = 0x55534101; // Enable write protection */
    HAL_EVETransferEnd();
    HAL_EVETransfer(0);
    return 0;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  void HAL_EVEClose()
  {
    PMC->PMC_PCER0 = BIT(ID_USART1); // USART1 disable */
    ResourceUnlock(RESOURCE_OWNER_EVE);
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  void HAL_EVESetSpeed(uint32_t baud)
  {
    USART1->US_WPMR = 0x55534100; // Disable write protection */
    uint32_t t = SystemCoreClock / (uint32_t) baud;
    USART1->US_BRGR = t & ~1;
    USART1->US_WPMR = 0x55534101; // Enable write protection */
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  void HAL_EVETransferBegin(void)
  {
    (void) USART1->US_RHR;    // Dummy read
    (void) USART1->US_RHR;    // Dummy read
    PIOA->PIO_CODR = BIT(24); // CS = 0
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  uint8_t HAL_EVETransfer(uint8_t send)
  {
    USART1->US_THR = send;                          // Send byte 1
    while ((USART1->US_CSR & US_CSR_TXRDY) == 0);   // Wait for complete
    while ((USART1->US_CSR & US_CSR_TXEMPTY) == 0); // Wait for transfer complete
    return USART1->US_RHR;
  }
#endif
/*---------------------------------------------------------------------------*/
#ifdef HAS_EVE
  void HAL_EVETransferEnd(void)
  {
    //while ((USART1->US_CSR & US_CSR_TXEMPTY) == 0); // Wait for transfer complete
    PIOA->PIO_SODR = BIT(24); // CS = 1
  }
#endif
/*---------------------------------------------------------------------------
  SoftPWM
  ---------------------------------------------------------------------------*/
typedef struct SoftPWMChannel {
  volatile uint32_t timerNS;
  volatile uint32_t intervalNS[2];
  bool high;
  Pio *pio;
  uint32_t pinMask;
} SoftPWMChannel;
/*---------------------------------------------------------------------------*/
typedef struct SoftPWM {
  uint32_t stepNS;
  uint8_t channelCount;
  SoftPWMChannel channels[8];
} SoftPWM;
SoftPWM softPWM;
/*---------------------------------------------------------------------------*/
int16_t HAL_SoftPWMInit(uint32_t baseClockHz, uint16_t channelCount)
{
  memset(&softPWM, 0, sizeof(softPWM));
  if ((baseClockHz < 1E3) || (baseClockHz > 0.5E6)) // 1kHz..500kHz
    return RESULT_HAL_ERROR_WRONG_PARAMETER;
  if (channelCount > 7)
    return RESULT_HAL_ERROR_WRONG_PARAMETER;
  softPWM.channelCount = channelCount;
  uint16_t threshold = SystemCoreClock / 2 / baseClockHz; // 60..30000
  //softPWM.stepNS = 1.0 / (double) SystemCoreClock * 1.0E9 * 2.0 * threshold;
  softPWM.stepNS = (uint64_t) 2E9 * (uint64_t) threshold / (uint64_t) SystemCoreClock;
  PMC->PMC_PCER0 = BIT(ID_TC0);  // TC0 enable
  TC0->TC_WPMR = 0x54494D00;     // Disables the Write Protect
  TC0->TC_CHANNEL[0].TC_RC = threshold;
  TC0->TC_CHANNEL[0].TC_CMR =
    TC_CMR_TCCLKS_TIMER_CLOCK1 | // MCK/2
    TC_CMR_WAVSEL_UP_RC |
    TC_CMR_WAVE;                 // Waveform Mode is enabled
  TC0->TC_CHANNEL[0].TC_IER =
    TC_SR_CPCS;                  // Enables the RC Compare Interrupt
  TC0->TC_CHANNEL[0].TC_IDR =
    ~TC_SR_CPCS;                 // Disable all other
  TC0->TC_CHANNEL[0].TC_CCR =
    TC_CCR_CLKEN |               // Counter Clock Enable Command
    TC_CCR_SWTRG;                // Software Trigger Command
  TC0->TC_WPMR = 0x54494D01;     // Enables the Write Protect
  NVIC->ISER[0] = BIT(TC0_IRQn); // Enable TC0 interrupts
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HAL_SoftPWMDeinit(void)
{
  NVIC->ICER[0] = BIT(TC0_IRQn); // Disble TC0 interrupts
  PMC->PMC_PCDR0 = BIT(ID_TC0);  // TC0 disable
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SoftPWMConfig(uint16_t channelNumber, int16_t pinId)
{
  if (channelNumber >= softPWM.channelCount)
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  SoftPWMChannel *channel = &softPWM.channels[channelNumber];
  channel->pio = HAL_GetPinPio(pinId);
  channel->pinMask = HAL_GetPinMask(pinId);
  channel->timerNS = 0;
  channel->intervalNS[0] = (uint32_t) -1;
  channel->intervalNS[1] = (uint32_t) -1;
  channel->pio->PIO_OER = channel->pinMask; // PIO Output Enable Register
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SoftPWMSet(uint16_t channelNumber, uint32_t intervalLowNS, uint32_t intervalHighNS)
{
  if (channelNumber >= softPWM.channelCount)
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  SoftPWMChannel *channel = &softPWM.channels[channelNumber];
  channel->intervalNS[0] = intervalLowNS;
  channel->intervalNS[1] = intervalHighNS;
  if ((intervalLowNS < softPWM.stepNS) && (intervalHighNS < softPWM.stepNS))
    return RESULT_HAL_ERROR_WRONG_PARAMETER;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void TC0_Handler(void)
{
  (void) TC0->TC_CHANNEL[0].TC_SR;
  SoftPWMChannel *channel = &softPWM.channels[0];
  for (uint8_t i = 0; i < softPWM.channelCount; i++) {
    register uint32_t timerNS = channel->timerNS;
    timerNS += softPWM.stepNS;
    if (channel->high) {
      if (timerNS >= channel->intervalNS[1]) {
        timerNS -= channel->intervalNS[1];
        channel->high = false;
        if (channel->pio)
          channel->pio->PIO_CODR = channel->pinMask; // PIO Clear Output Data Register
      }
    } else {
      if (timerNS >= channel->intervalNS[0]) {
        timerNS -= channel->intervalNS[0];
        channel->high = true;
        if (channel->pio)
          channel->pio->PIO_SODR = channel->pinMask; // PIO Set Output Data Register
      }
    }
    channel->timerNS = timerNS;
    channel++;
  }
}
/*---------------------------------------------------------------------------
  Soft counter
  ---------------------------------------------------------------------------*/
static SoftCounter* SoftCountFind(int16_t pinId)
{
  for (uint8_t i = 0; i <= 7; i++)
    if (hal.counter[i].pinId == pinId)
      return &hal.counter[i];
  return 0;
}
/*---------------------------------------------------------------------------*/
static SoftCounter* SoftCountFindFree(void)
{
  for (uint8_t i = 0; i <= 7; i++)
    if (hal.counter[i].pinId < 0)
      return &hal.counter[i];
  return 0;
}
/*---------------------------------------------------------------------------*/
void HAL_SoftCounterInit(void)
{
  HAL_SoftCounterDeinit();
  NVIC->ISER[0] = BIT(PIOA_IRQn) | BIT(PIOB_IRQn); 
  (void) PIOA->PIO_ISR;
  (void) PIOB->PIO_ISR;
}
/*---------------------------------------------------------------------------*/
void HAL_SoftCounterDeinit(void)
{
  memset(hal.counter, 0, sizeof(hal.counter));
  for (uint8_t i = 0; i <= 7; i++)
    hal.counter[i].pinId = -1;
  PIOA->PIO_IDR = 0xFFFFFFFF;
  PIOB->PIO_IDR = 0xFFFFFFFF;
  NVIC->ICER[0] = BIT(PIOA_IRQn) | BIT(PIOB_IRQn); 
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SoftCounterConfig(int16_t pinId, const char *config, uint8_t configNumber)
{
  bool Match(const char *text)
  {
    int length = strlen(config);
    return length && (strncmp(config, text, length) == 0);
  }

  SoftCounter *s = SoftCountFind(pinId);
  if (!s) {
    s = SoftCountFindFree();
    s->pinId = pinId;
  }
  if (!s)
    return RESULT_HAL_ERROR_NO_MORE_CHANNELS;
  Pio *pio = HAL_GetPinPio(pinId);
  uint32_t pinMask = HAL_GetPinMask(pinId);
  if (Match("FREE")) {
    pio->PIO_IDR = pinMask;        // Interrupt Disable Register
  } else { 
    if (pio->PIO_LOCKSR & pinMask) // Check if the pin lock by different periperie
      return RESULT_HAL_ERROR_RESOURCE_IN_USE;
    pio->PIO_ESR = pinMask;        // Edge Select Register
    if (Match("CHANGE")) {
      pio->PIO_AIMDR = pinMask;    // Additional Interrupt Modes Disable Register
    } else if (Match("RISE")) {
      pio->PIO_AIMER = pinMask;    // Additional Interrupt Modes Enable Register
      pio->PIO_REHLSR = pinMask;   // Rising Edge/High-Level Select Register
    } else if (Match("FALL")) {
      pio->PIO_AIMER = pinMask;    // Additional Interrupt Modes Enable Register
      pio->PIO_FELLSR = pinMask;   // Falling Edge/Low-Level Select Register
    } else
      return RESULT_HAL_ERROR_WRONG_PARAMETER;
    pio->PIO_IER = pinMask;        // Interrupt Enable Register
  }
  return RESULT_OK;  
}
/*---------------------------------------------------------------------------*/
int16_t HAL_SoftCounterGet(int16_t pinId, int64_t *actual, int64_t *set)
{
  SoftCounter *s = SoftCountFind(pinId);
  if (!s)
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  GlobalInterruptsDisable();
  *actual = s->count;
  if (set)
    s->count = *set;
  GlobalInterruptsEnable();
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void PIOA_Handler(void)
{
  uint32_t status = PIOA->PIO_ISR;
  SoftCounter *s = &hal.counter[0];
  for (uint8_t i = 0; i <= 7; i++) {
    if (!(s->pinId & BIT(5)) && (status & HAL_GetPinMask(s->pinId))) 
      s->count++;
    s++;
  }
}
/*---------------------------------------------------------------------------*/
void PIOB_Handler(void)
{
  uint32_t status = PIOB->PIO_ISR;
  SoftCounter *s = &hal.counter[0];
  for (uint8_t i = 0; i <= 7; i++) {
    if ((s->pinId & BIT(5)) && (status & HAL_GetPinMask(s->pinId))) 
      s->count++;
    s++;
  }
}
/*---------------------------------------------------------------------------
  ADC
  ---------------------------------------------------------------------------*/
static int16_t ADChannelToPinId[10] = {
  PINID('A', 17), PINID('A', 18), PINID('A', 19), PINID('A', 20), 
  PINID('B', 0), PINID('B', 1), PINID('B', 2), PINID('B', 3),
  PINID('A', 12), PINID('A', 22)
};
/*---------------------------------------------------------------------------*/
int16_t HAL_ADCInit(bool autoCalibrate) //TODO Wandlerate einstellbar machen
{
  PMC->PMC_PCER0 = BIT(ID_ADC);   // ADC enable 
  ADC->ADC_WPMR = 0x41444300;     // Disables the Write Protect */
  ADC->ADC_CR = ADC_CR_SWRST;     // Reset the ADC */
  ADC->ADC_CR = ADC_CR_SWRST;     // Reset the ADC */
  if (autoCalibrate)
    ADC->ADC_CR = ADC_CR_AUTOCAL;
  ADC->ADC_MR =
    (5 << ADC_MR_PRESCAL_Pos) |   // ADCClock = MCK / ((PRESCAL + 1) * 2) -> 10MHz */
    (8 << ADC_MR_STARTUP_Pos) |   // 512 periods of ADCClock */
    (2 << ADC_MR_SETTLING_Pos) |  // Analog Settling Time 9 periods of ADCClock */
    (15 << ADC_MR_TRACKTIM_Pos) | // Tracking Time = (TRACKTIM + 1) * ADCClock periods  */
    (2 << ADC_MR_TRANSFER_Pos) |
    ADC_MR_FREERUN;               // Free Run Mode */
  ADC->ADC_IDR = 0xFFFFFFFF;      // Disable all interrupts */
  ADC->ADC_EMR = ADC_EMR_TAG;     // Appends the channel number to the conversion result in ADC_LDCR */ 
  ADC->ADC_CGR = 0x11111111;      // Gain of 1 for all channels */
  ADC->ADC_COR = 0;               // No offsets */
  ADC->ADC_CHDR = 0xFFFF;         // Disable all channels */
  ADC->ADC_CWR = 0x0FFF0000;      // low = 0, high = 4095 */
  ADC->ADC_ACR = 0;
  ADC->ADC_CR = ADC_CR_START;     // Start the ADC */
  ADC->ADC_WPMR = 0x41444301;     // Enables the Write Protect */
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
void HAL_ADCDeinit(void)
{
  PMC->PMC_PCDR0 = BIT(ID_ADC);   // ADC disable */
}
/*---------------------------------------------------------------------------*/
int16_t HAL_ADCConfig(uint16_t channelNumber, const char *config, uint8_t configNumber)
{
  bool Match(const char *text)
  {
    int length = strlen(config);
    return length && (strncmp(config, text, length) == 0);
  }
  uint32_t Mask2(uint8_t value)
  {
    return (value & 0b11) << (channelNumber << 1);
  }

  if (channelNumber > 9)
    return RESULT_HAL_ERROR_WRONG_CHANNEL;
  ADC->ADC_WPMR = 0x41444300;     // Disables the Write Protect */
  while (ADC->ADC_ISR & ADC_ISR_EOCAL) {}
  if (Match("FREE")) {
    ADC->ADC_CHDR = BIT(channelNumber);
    ResourceUnlock(ADChannelToPinId[channelNumber]);
  } else {
    if (!configNumber) {
      int16_t result = ResourceLock1(RESOURCE_OWNER_ADC, ADChannelToPinId[channelNumber]);
      if (result < 0)
        return result;
    }
    ADC->ADC_CHER = BIT(channelNumber);
    if (Match("GAIN1")) {
      ADC->ADC_CGR &= ~Mask2(3);
    } else if (Match("GAIN2")) {
      ADC->ADC_CGR = (ADC->ADC_CGR & ~Mask2(3)) | ~Mask2(2);
    } else if (Match("GAIN4")) {
      ADC->ADC_CGR |= Mask2(3);
    }
    if (Match("BIPOLAR"))
      ADC->ADC_COR |= BIT(channelNumber);
    if (Match("UNIPOLAR"))
      ADC->ADC_COR &= ~BIT(channelNumber);
  }
  ADC->ADC_WPMR = 0x41444301; // Enables the Write Protect 
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_ADCGet(uint16_t channelNumber, float *value)
{
  if (!(ADC->ADC_CHSR & BIT(channelNumber)))
    return RESULT_HAL_ERROR_WRONG_PARAMETER;
  int32_t ad = ADC->ADC_CDR[channelNumber];
  if (ADC->ADC_COR & BIT(channelNumber))
    ad -= 2048;
  *value = (float) ad / 4095.0;
  return RESULT_OK;
}
/*---------------------------------------------------------------------------*/
int16_t HAL_ADCGetAll(float *values)
{
  uint8_t n = 0; 
  for (uint8_t i = 0; i <= 9; i++) 
    if (ADC->ADC_CHSR & BIT(i)) {
      if (values) {
        int16_t result = HAL_ADCGet(i, values++);
        if (result < 0)
          return result;
      }
      n++;
    }
  return n;
}
/*---------------------------------------------------------------------------
  HAL
  ---------------------------------------------------------------------------*/
void HAL_Init(void)
{
  DEBUG("HAL: Init\n");

  memset((void*) &hal, 0, sizeof(hal));
  hal.preGuard = 123456789;
  hal.postGuard = 123456789;
  hal.watchdogAutoServeKey = 0xDECAFBAD; 

  PMC->PMC_WPMR = 0x504D4300; // Disable the Write Protection */
  PIOA->PIO_WPMR = 0x50494F00;
  PIOB->PIO_WPMR = 0x50494F00;

  // PMC Peripheral Clock Enable Register 0 */
  PMC->PMC_PCER0 =
    BIT(ID_PIOA) |  // PIOA enabled */
    BIT(ID_PIOB);   // PIOB enabled */
    //BIT(ID_HSMCI); /////////////

  // PIO Output Disable Register */
  PIOA->PIO_ODR = 0xFFFFFFFF;
  PIOB->PIO_ODR = 0xFFFFFFFF;

  // PIO Input Filter Disable Register */
  PIOA->PIO_IFDR = 0xFFFFFFFF;
  PIOB->PIO_IFDR = 0xFFFFFFFF;

  // PIO Interrupt Disable Register */
  PIOA->PIO_IDR = 0xFFFFFFFF;
  PIOB->PIO_IDR = 0xFFFFFFFF;

  // PIO Pull Up Disable Register */
  PIOA->PIO_PUDR = 0xFFFFFFFF;
  PIOB->PIO_PUDR = 0xFFFFFFFF;

  // PIO Pad Pull Down Disable Register */
  PIOA->PIO_PPDDR = 0xFFFFFFFF;
  PIOB->PIO_PPDDR = 0xFFFFFFFF;

  // PIO Multi-driver Disable Register */
  PIOA->PIO_MDDR = 0xFFFFFFFF;
  PIOB->PIO_MDDR = 0xFFFFFFFF;

  // PIO Parallel Capture Mode Register */
  PIOA->PIO_PCMR = 0;
  PIOB->PIO_PCMR = 0;

  // Peripheral ABCD Select set to peripheral A function */
  PIOA->PIO_ABCDSR[0] = 0;
  PIOA->PIO_ABCDSR[1] = 0;
  PIOB->PIO_ABCDSR[0] = 0;
  PIOB->PIO_ABCDSR[1] = 0;

  #if 0
  NVIC->NVIC_IPR0 = 0xFFFFFFFF;
  NVIC_IPR1 = 0xFFFFFFFF;
  NVIC_IPR2 = 0xFFFFFFFF;
  NVIC_IPR3 = 0xFFFFFFFF;
  NVIC_IPR4 = 0xFFFFFFFF;

  NVIC_ICPR0 = 0xFFFFFFFF;
  NVIC_ICPR1 = 0xFFFFFFFF;
  #endif

  HAL_SysLEDInit();
  HAL_SysTickInit();
  HAL_SPIEthInit();
  HAL_SerialInit();

  HAL_EthInit();
  #ifdef HAS_ATSHA204
    HAL_ATSHA204Init();
  #endif
  #ifdef HAS_I2C
    memset(&i2c, 0, sizeof(i2c));
  /*
    HAL_I2COpen(0);
    for (;;) {
      //for (uint8_t addr = 10; addr < 120; addr++) {
      for (uint8_t addr = 30; addr < 35; addr++) {
        if (HAL_I2CExist(0, addr) == RESULT_OK) 
          debug_printf(" %d", addr);
        else
          debug_printf(" -");
      }
      debug_printf("\n");
      /*
      uint8_t pat[4] = {0x55,0x55,0x00,0xFF};
      for (;;) {
        HAL_I2CSend(0, 32, 0x010203, 3, &pat, 4);
        //HAL_DelayMS(100);
        pat[0]++;
      }
      * /
      uint8_t pat = 0xFF;
      HAL_I2CSend(0, 32, 0, 0, &pat, 1);
      for (;;) {
        uint8_t pat = 0;
        int16_t res = HAL_I2CRecv(0, 32, 0, 0, &pat, 1);
        HAL_DelayMS(100);
        debug_printf("%d %d\n", res, pat);
      }
    }
  */
  #endif
  //PMC->PMC_WPMR = 0x504D4301; // Enable the Write Protection */
}
/*---------------------------------------------------------------------------*/
#if 0
  PMC->PMC_WPMR = 0x504D4300; // Disable the Write Protection */
  PMC->PMC_WPMR = 0x504D4301; // Enable the Write Protection */
#endif

