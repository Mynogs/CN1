# CN1
IDE: CrossStudio for ARM 2.3 "http://www.rowley.co.uk/arm/"
Project settings:
 To use Segger J-Link set path to JLinkARM.dll

Modify file:
  C:\Users\User\AppData\Local\Rowley Associates Limited\CrossWorks for ARM\packages\targets\SAM\CMSIS\Device\ATMEL\sam4s\source\system_sam4s.c
at line 49:

/ * Clock settings (120MHz) * /
// AR
#if CHIP_FREQ_XTAL == 12000000UL
  #define SYS_BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8UL))
  #define SYS_BOARD_PLLAR     (CKGR_PLLAR_ONE | CKGR_PLLAR_MULA(0x13UL) | CKGR_PLLAR_PLLACOUNT(0x3fUL) | CKGR_PLLAR_DIVA(0x1UL))
  #define SYS_BOARD_MCKR      (PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)
#elif CHIP_FREQ_XTAL == 16000000UL
  #define SYS_BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8UL))
  #define SYS_BOARD_PLLAR     (CKGR_PLLAR_ONE | CKGR_PLLAR_MULA(0x0EUL) | CKGR_PLLAR_PLLACOUNT(0x3fUL) | CKGR_PLLAR_DIVA(0x1UL))
  #define SYS_BOARD_MCKR      (PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)
#else
  #error Unknow crital frequency
#endif
