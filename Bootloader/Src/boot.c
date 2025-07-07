/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
#define RESERVED_OTA_SECTORS             96 /* Number of sectors reserved for HOTA */
#define RESERVED_BOOT_SECTORS            8  /* Bootloader size in sectors */
#define APP_START_ADDRESS                (FLASH_BASE + (FLASH_SECTOR_SIZE * RESERVED_BOOT_SECTORS))
#define HOTA_START_ADDRESS               (FLASH_BASE + FLASH_BANK_SIZE)
#define SCRATCH_SECTOR                   (RESERVED_OTA_SECTORS + RESERVED_BOOT_SECTORS)
#define SCRATCH_SECTOR_BANK              FLASH_BANK_1

#define NUM_QUAD_WORDS( length )         ( length >> 4UL )
#define NUM_REMAINING_BYTES( length )    ( length & 0x0F )
#define vPetWatchdog()

#define SWAP_APPLICATION_AND_OTA 1
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;

/* Private function prototypes -----------------------------------------------*/
static bool prvWriteToFlash(uint8_t *pSource, uint32_t destination,  uint32_t ulLength);
static bool prvEraseSectors(uint32_t Nb_Bank, uint32_t Nb_Sectors, uint32_t StartSector);
static void copy_from_flash_to_buffer(uint8_t *dest, const uint32_t src_flash_addr, uint32_t size);


static bool prvEraseApplication(void);
static bool prvEraseOTA(void);

/* Private user code ---------------------------------------------------------*/

void Jump_To_Main_Application(void)
{
  /* Verify APP_START_ADDRESS contains expected initial SP */
  if (*(uint32_t*)APP_START_ADDRESS != 0x200A0000)
  {
    /* Handle error: invalid address or corrupted application */
    return;
  }

  HAL_SuspendTick();

#if defined(HAL_ICACHE_MODULE_ENABLED)
  HAL_ICACHE_Disable();
#endif

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  uint32_t saved_flash_latency = __HAL_FLASH_GET_LATENCY();
  __HAL_FLASH_SET_LATENCY(__HAL_FLASH_GET_LATENCY() + 2);
  (void)__HAL_FLASH_GET_LATENCY();
#endif

  /* Jump to user application */
  JumpAddress = *(uint32_t*)(APP_START_ADDRESS + 4);
  JumpToApplication = (pFunction)JumpAddress;

  /* Initialize user application's Stack Pointer */
  __set_MSP(*(uint32_t*)APP_START_ADDRESS);

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  __HAL_FLASH_SET_LATENCY(saved_flash_latency);
  (void)__HAL_FLASH_GET_LATENCY();
#endif

  JumpToApplication();
}

bool check_for_hota(void)
{
  bool xReseult = false;
#if defined(HAL_ICACHE_MODULE_ENABLED)
HAL_ICACHE_Disable();
#endif

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  uint32_t saved_flash_latency = __HAL_FLASH_GET_LATENCY();
  __HAL_FLASH_SET_LATENCY(__HAL_FLASH_GET_LATENCY() + 2);
  (void) __HAL_FLASH_GET_LATENCY();
#endif

  // Create a pointer to the HOTA_START_ADDRESS
  uint32_t *address_ptr = (uint32_t*) HOTA_START_ADDRESS;

  // Read the value at that address
  uint32_t value = *address_ptr;

  // Check if the value is different from 0xFFFFFFFF
  xReseult = (value != 0xFFFFFFFFU);

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  __HAL_FLASH_SET_LATENCY(saved_flash_latency);
  (void) __HAL_FLASH_GET_LATENCY();
#endif

#if defined(HAL_ICACHE_MODULE_ENABLED)
HAL_ICACHE_Enable();
#endif

  return xReseult;
}

#if (SWAP_APPLICATION_AND_OTA == 1)
uint8_t dst_buffer    [FLASH_SECTOR_SIZE];
#endif
uint8_t src_buffer    [FLASH_SECTOR_SIZE];

void copy_hota(void)
{
  uint32_t total_sectors = RESERVED_OTA_SECTORS;
  uint32_t dst_address = 0;     /* first APP sector address */
  uint32_t src_address = 0;

#if (SWAP_APPLICATION_AND_OTA == 1)
  uint32_t dest_sector = RESERVED_BOOT_SECTORS; /* APP sectors start after bootloader sectors                */
  uint32_t dest_bank   = FLASH_BANK_1;

  uint32_t src_sector  = 0;                     /* HOTA sectors start at sector 0 in bank 1                  */
  uint32_t src_bank    = FLASH_BANK_2;
#endif


#if (SWAP_APPLICATION_AND_OTA == 0)
  prvEraseApplication();
#endif

  /* Swap sectors */
  for (uint32_t sector_idx = 0; sector_idx < total_sectors; sector_idx++)
  {
    src_address = HOTA_START_ADDRESS + (sector_idx * FLASH_SECTOR_SIZE);
    dst_address = APP_START_ADDRESS  + (sector_idx * FLASH_SECTOR_SIZE);

    /* Copy source sector (HOTA) to buffer */
    copy_from_flash_to_buffer(src_buffer, src_address, FLASH_SECTOR_SIZE);

#if (SWAP_APPLICATION_AND_OTA == 1)
    copy_from_flash_to_buffer(dst_buffer, dst_address, FLASH_SECTOR_SIZE);


    /* Erase destination sector before writing */
    if (!prvEraseSectors(dest_bank, 1, dest_sector + sector_idx))
    {
      return;
    }
#endif
    /* Write buffer to destination sector (APP) */
    if (!prvWriteToFlash(src_buffer, dst_address,FLASH_SECTOR_SIZE))
    {
      return;
    }

#if (SWAP_APPLICATION_AND_OTA == 1)
    /* Erase source sector */
    if (!prvEraseSectors(src_bank, 1, src_sector + sector_idx))
    {
      return;
    }


    if (!prvWriteToFlash(dst_buffer, src_address, FLASH_SECTOR_SIZE))
    {
      return;
    }
#endif
  }

#if (SWAP_APPLICATION_AND_OTA == 0)
  prvEraseOTA();
#endif
}

/**
 * @brief Copies data from flash memory to a destination buffer.
 *
 * @param dest Pointer to the destination buffer in RAM.
 * @param src_flash_addr Source address in flash memory.
 * @param size Number of bytes to copy.
 */
static void copy_from_flash_to_buffer(uint8_t *dest, const uint32_t src_flash_addr, uint32_t size)
{
#if defined(HAL_ICACHE_MODULE_ENABLED)
HAL_ICACHE_Disable();
#endif

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  uint32_t saved_flash_latency = __HAL_FLASH_GET_LATENCY();
  __HAL_FLASH_SET_LATENCY(__HAL_FLASH_GET_LATENCY() + 2);
  (void) __HAL_FLASH_GET_LATENCY();
#endif

  const uint8_t *src_ptr = (const uint8_t*) src_flash_addr;
  memcpy(dest, src_ptr, size);

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  __HAL_FLASH_SET_LATENCY(saved_flash_latency);
  (void) __HAL_FLASH_GET_LATENCY();
#endif

#if defined(HAL_ICACHE_MODULE_ENABLED)
HAL_ICACHE_Enable();
#endif
}

static bool prvWriteToFlash(uint8_t *pSource, uint32_t destination,  uint32_t ulLength)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t i = 0U;
  uint8_t quadWord[16] = { 0 };
  uint32_t numQuadWords = NUM_QUAD_WORDS(ulLength);
  uint32_t remainingBytes = NUM_REMAINING_BYTES(ulLength);

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  for (i = 0U; i < numQuadWords; i++)
  {
    /* Pet the watchdog */
    vPetWatchdog();

    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
     * be done by word */

    memcpy(quadWord, pSource, 16UL);
    status = HAL_FLASH_Program( FLASH_TYPEPROGRAM_QUADWORD, destination, (uint32_t) quadWord);

    if (status == HAL_OK)
    {
      /* Check the written value */
      if (memcmp((void*) destination, quadWord, 16UL) != 0)
      {
        /* Flash content doesn't match SRAM content */
        status = HAL_ERROR;
      }
    }

    if (status == HAL_OK)
    {
      /* Increment FLASH destination address and the source address. */
      destination += 16UL;
      pSource += 16UL;
    }
    else
    {
      break;
    }
  }

  if (status == HAL_OK)
  {
    if (remainingBytes > 0)
    {
      memcpy(quadWord, pSource, remainingBytes);
      memset((quadWord + remainingBytes), 0xFF, (16UL - remainingBytes));

      status = HAL_FLASH_Program( FLASH_TYPEPROGRAM_QUADWORD, destination, (uint32_t) quadWord);

      if (status == HAL_OK)
      {
        /* Check the written value */
        if (memcmp((void*) destination, quadWord, 16UL) != 0)
        {
          /* Flash content doesn't match SRAM content */
          status = HAL_ERROR;
        }
      }
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
   *  to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return status == HAL_OK;
}

static bool prvEraseSectors(uint32_t Nb_Bank, uint32_t Nb_Sectors, uint32_t StartSector)
{
  bool xResult = true;

#if defined(HAL_ICACHE_MODULE_ENABLED)
    HAL_ICACHE_Disable();
    #endif

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  uint32_t saved_flash_latency = __HAL_FLASH_GET_LATENCY();
  __HAL_FLASH_SET_LATENCY(__HAL_FLASH_GET_LATENCY() + 2);
  (void) __HAL_FLASH_GET_LATENCY();
#endif

  if (HAL_FLASH_Unlock() == HAL_OK)
  {
    uint32_t pageError = 0U;
    FLASH_EraseInitTypeDef pEraseInit;
#if defined(STM32H5)
    pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    pEraseInit.Banks     = Nb_Bank;
    pEraseInit.NbSectors = Nb_Sectors;
    pEraseInit.Sector    = StartSector;
#else
    pEraseInit.TypeErase = FLASH_TYPEERASE_BANKS;
    pEraseInit.Banks     = FLASH_BANK_1;
    pEraseInit.NbPages   = RESERVED_OTA_SECTORS;
    pEraseInit.Page      = RESERVED_BOOT_SECTORS;
#endif

    if (HAL_FLASHEx_Erase(&pEraseInit, &pageError) != HAL_OK)
    {
      PRINTF_ERROR("Failed to erase the flash bank, errorCode = %u, pageError = %u.", HAL_FLASH_GetError(), pageError);
      xResult = false;
    }

    (void) HAL_FLASH_Lock();
  }
  else
  {
    PRINTF_ERROR("Failed to lock flash for erase, errorCode = %u.", HAL_FLASH_GetError());
    xResult = false;
  }

#if defined(__STM32H5xx_HAL_H) || defined(STM32L5xx_HAL_H)
  __HAL_FLASH_SET_LATENCY(saved_flash_latency);
  (void) __HAL_FLASH_GET_LATENCY();
  ;
#endif

#if defined(HAL_ICACHE_MODULE_ENABLED)
    HAL_ICACHE_Enable();
    #endif

  return xResult;
}

static bool prvEraseApplication(void)
{
  return prvEraseSectors(FLASH_BANK_1, RESERVED_OTA_SECTORS, RESERVED_BOOT_SECTORS) == HAL_OK;
}

static bool prvEraseOTA(void)
{
  return prvEraseSectors(FLASH_BANK_2, RESERVED_OTA_SECTORS, 0) == HAL_OK;
}

