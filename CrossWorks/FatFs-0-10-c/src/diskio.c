/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"    /* FatFs lower layer API */

#include "../HAL/HAL.h"
#include "../HAL/HALHSMCI.h"
#include "../TFTPBoot.h"
#include "../FFS.h"

/* Definitions of physical drive number for each drive */
#define FLASH    0
#define MMC      1
#define FFS      2

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
  BYTE pdrv    /* Physical drive nmuber to identify the drive */
)
{
  switch (pdrv) {
    case FLASH :
      return *TFTPBoot_GetFlashImageAddress() != 0xFF ? 0 : STA_NOINIT;

    case MMC :
      return HAL_GetSDCardState() & 1 ? 0 : STA_NODISK;

    case FFS :
      return FFS_GetState() ? 0 : STA_NODISK;

  }
  return STA_NOINIT;
}
/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
  BYTE pdrv        /* Physical drive nmuber to identify the drive */
)
{
  switch (pdrv) {
  case FLASH :
    return *TFTPBoot_GetFlashImageAddress() != 0xFF ? 0 : STA_NOINIT;

  case MMC :
    return 0;

  case FFS :
    return 0;

  }
  return STA_NOINIT;
}
/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
  BYTE pdrv,    /* Physical drive nmuber to identify the drive */
  BYTE *buff,    /* Data buffer to store read data */
  DWORD sector,  /* Sector address in LBA */
  UINT count    /* Number of sectors to read */
)
{
  switch (pdrv) {

    case FLASH :
      memcpy(buff, TFTPBoot_GetFlashImageAddress() + sector * 512, count * 512);
      return 0;

    case MMC :
      return HALHSMCI_ReadBlocks(buff, sector, count) >= 0 ? RES_OK : RES_ERROR;

    case FFS :
      return FFS_ReadBlocks(buff, sector, count) >= 0 ? RES_OK : RES_ERROR;

  }
  return RES_PARERR;
}
/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
  BYTE pdrv,      /* Physical drive nmuber to identify the drive */
  const BYTE *buff,  /* Data to be written */
  DWORD sector,    /* Sector address in LBA */
  UINT count      /* Number of sectors to write */
)
{
  switch (pdrv) {

    case FLASH :
      return RES_ERROR;

    case MMC :
      return HALHSMCI_WriteBlocks(buff, sector, count) >= 0 ? RES_OK : RES_ERROR;

    case FFS :
      return FFS_WriteBlocks(buff, sector, count) >= 0 ? RES_OK : RES_ERROR;

  }
  return RES_PARERR;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
  BYTE pdrv,    /* Physical drive nmuber (0..) */
  BYTE cmd,    /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
  DRESULT res;
  int result;

  switch (pdrv) {
  case FLASH :

    // Process of the command for the ATA drive

    return res;

  case MMC :

    // Process of the command for the MMC/SD card
    return res;

  }

  return RES_PARERR;
}
#endif
