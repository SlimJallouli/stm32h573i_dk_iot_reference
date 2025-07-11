
/**
  ******************************************************************************
  * @file           : stsafea_crypto_interface.c
  * @brief          : Crypto Interface file to support the crypto services required by the
  *                   STSAFE-A BSP and offered by the MbedTLS library:
  *                     + Key Management
  *                     + SHA
  *                     + AES
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "stsafea_crypto.h"
#include "safea1_conf.h"
#include "main.h"

#if (USE_SIGNATURE_SESSION)
#include "sha256.h"
#include "sha512.h"
#endif /* USE_SIGNATURE_SESSION */

#include "mbedtls/aes.h"
#include "mbedtls/cmac.h"
/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#if (USE_PRE_LOADED_HOST_KEYS)
static uint8_t  aHostCipherKey[STSAFEA_HOST_KEY_LENGTH]; /* STSAFE-A's Host cipher key */
static uint8_t  aHostMacKey   [STSAFEA_HOST_KEY_LENGTH]; /* STSAFE-A's Host Mac key */
#else
static uint8_t  aHostCipherKey[] = {0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55,
                                    0x66, 0x66, 0x77, 0x77, 0x88, 0x88}; /* STSAFE-A's Host cipher key */
static uint8_t  aHostMacKey   [] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                                    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; /* STSAFE-A's Host Mac key */
#endif /* USE_PRE_LOADED_HOST_KEYS */

#if (USE_SIGNATURE_SESSION)

#ifdef MBEDTLS_SHA256_C
static mbedtls_sha256_context         sha256_ctx;
#endif /* MBEDTLS_SHA256_C */
#ifdef MBEDTLS_SHA512_C
static mbedtls_sha512_context         sha512_ctx;
#endif /* MBEDTLS_SHA512_C */
#endif /* USE_SIGNATURE_SESSION */

#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
static mbedtls_cipher_context_t       cipher_ctx;
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

/** @addtogroup CRYPTO_IF_Exported_Functions_Group1 Host MAC and Cipher keys Initialization
  *  @brief    Crypto Interface APIs to be implemented at application level. Templates are provided.
  *
@verbatim
 ===============================================================================
           ##### Host MAC and Cipher keys Initialization functions #####
 ===============================================================================

@endverbatim
  * @{
  */

/**
  * @brief   StSafeA_HostKeys_Init
  *          Initialize STSAFE-Axxx Host MAC and Cipher Keys that will be used by the crypto interface layer
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with BSP
  *
  * @param   None
  * @retval  0 if success. An error code otherwise
  */
int32_t StSafeA_HostKeys_Init()
{
#if (USE_PRE_LOADED_HOST_KEYS)
  /* This is just a very easy example to retrieve keys pre-loaded at the end of the MCU Flash
     and load them into the SRAM. Host MAC and Cipher Keys are previously pre-stored at the end
     of the MCU flash (e.g. by the SDK Pairing Application example) .
     It's up to the user to protect the MAC and Cipher keys and to find the proper
     and most secure way to retrieve them when needed or to securely keep them into
     a protected volatime memory during the application life */
//  pkcs11configLABEL_HMAC_KEY
  /* Host MAC Key */
  uint32_t host_mac_key_addr = FLASH_BASE + FLASH_SIZE - 2U * (STSAFEA_HOST_KEY_LENGTH);

//  pkcs11configLABEL_CMAC_KEY
  /* Host Cipher Key */
  uint32_t host_cipher_key_addr = FLASH_BASE + FLASH_SIZE - (STSAFEA_HOST_KEY_LENGTH);

  /* Set and keep the keys that will be used during the Crypto / MAC operations */
  (void)memcpy(aHostMacKey, (uint8_t *)host_mac_key_addr,    STSAFEA_HOST_KEY_LENGTH);
  (void)memcpy(aHostCipherKey, (uint8_t *)host_cipher_key_addr, STSAFEA_HOST_KEY_LENGTH);
#endif /* USE_PRE_LOADED_HOST_KEYS */

  return 0;
}

/**
  * @}
  */

#if (USE_SIGNATURE_SESSION)

/** @addtogroup CRYPTO_IF_Exported_Functions_Group2 HASH Functions
  *  @brief    Crypto Interface APIs to be implemented at application level. Templates are provided.
  *
@verbatim
 ===============================================================================
                          ##### HASH functions #####
 ===============================================================================

@endverbatim
  * @{
  */

/**
  * @brief   StSafeA_SHA_Init
  *          SHA initialization function to initialize the SHA context
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   ppShaCtx : SHA context to be initialized
  * @retval  None
  */
void StSafeA_SHA_Init(StSafeA_HashTypes_t InHashType, void **ppShaCtx)
{
  switch (InHashType)
  {

#ifdef MBEDTLS_SHA256_C
    case STSAFEA_SHA_256:
      *ppShaCtx = &sha256_ctx;
      mbedtls_sha256_init(*ppShaCtx);
      mbedtls_sha256_starts(*ppShaCtx, 0);
      break;
#endif /* MBEDTLS_SHA256_C */

#ifdef MBEDTLS_SHA512_C
    case STSAFEA_SHA_384:
      *ppShaCtx = &sha512_ctx;
      mbedtls_sha512_init(*ppShaCtx);
      mbedtls_sha512_starts(*ppShaCtx, 1);
      break;
#endif /* MBEDTLS_SHA512_C */

    default:
      break;
  }
}

/**
  * @brief   StSafeA_SHA_Update
  *          SHA update function to process SHA over a message data buffer.
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   pShaCtx : SHA context
  * @param   pInMessage : message data buffer
  * @param   InMessageLength : message data buffer length
  * @retval  None
  */
void StSafeA_SHA_Update(StSafeA_HashTypes_t InHashType, void *pShaCtx, uint8_t *pInMessage, uint32_t InMessageLength)
{

  switch (InHashType)
  {
#ifdef MBEDTLS_SHA256_C
    case STSAFEA_SHA_256:
      if ((pShaCtx != NULL) && (pInMessage != NULL))
      {
        mbedtls_sha256_update(pShaCtx, pInMessage, InMessageLength);
      }
      break;
#endif /* MBEDTLS_SHA256_C */

#ifdef MBEDTLS_SHA512_C
    case STSAFEA_SHA_384:
      if ((pShaCtx != NULL) && (pInMessage != NULL))
      {
        mbedtls_sha512_update(pShaCtx, pInMessage, InMessageLength);
      }
      break;
#endif /* MBEDTLS_SHA512_C */
    default:
      break;
  }
}

/**
  * @brief   StSafeA_SHA_Final
  *          SHA final function to finalize the SHA Digest
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   InHashType : type of SHA
  *          This parameter can be one of the StSafeA_HashTypes_t enum values:
  *            @arg STSAFEA_SHA_256: 256-bits
  *            @arg STSAFEA_SHA_384: 384-bits
  * @param   ppShaCtx : SHA context to be finalized
  * @param   pMessageDigest : message digest data buffer
  * @retval  None
  */
void StSafeA_SHA_Final(StSafeA_HashTypes_t InHashType, void **ppShaCtx, uint8_t *pMessageDigest)
{

  switch (InHashType)
  {
#ifdef MBEDTLS_SHA256_C
    case STSAFEA_SHA_256:
      if (*ppShaCtx != NULL)
      {
        if (pMessageDigest != NULL)
        {
          mbedtls_sha256_finish(*ppShaCtx, pMessageDigest);
        }
        mbedtls_sha256_free(*ppShaCtx);
        *ppShaCtx = NULL;
      }
      break;
#endif /* MBEDTLS_SHA256_C */

#ifdef MBEDTLS_SHA512_C
    case STSAFEA_SHA_384:
      if (*ppShaCtx != NULL)
      {
        if (pMessageDigest != NULL)
        {
          mbedtls_sha512_finish(*ppShaCtx, pMessageDigest);
        }
        mbedtls_sha512_free(*ppShaCtx);
        *ppShaCtx = NULL;
      }
      break;
#endif /* MBEDTLS_SHA512_C */

    default:
      break;
  }
}

#endif /* USE_SIGNATURE_SESSION */

/**
  * @}
  */

/** @addtogroup CRYPTO_IF_Exported_Functions_Group3 AES Functions
  *  @brief    Crypto Interface APIs to be implemented at application level. Templates are provided.
  *
@verbatim
 ===============================================================================
                          ##### AES functions #####
 ===============================================================================

@endverbatim
  * @{
  */

/**
  * @brief   StSafeA_AES_MAC_Start
  *          Start AES MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   ppAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Start(void **ppAesMacCtx)
{

#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
  *ppAesMacCtx = &cipher_ctx;

  mbedtls_cipher_init(*ppAesMacCtx);
  (void)mbedtls_cipher_setup(*ppAesMacCtx, mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB));
  (void)mbedtls_cipher_cmac_starts(*ppAesMacCtx, aHostMacKey, STSAFEA_HOST_KEY_LENGTH * 8U);
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
}

/**
  * @brief   StSafeA_AES_MAC_Update
  *          Update / Add data to MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pInData : data buffer
  * @param   InDataLength : data buffer length
  * @param   pAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Update(uint8_t *pInData, uint16_t InDataLength, void *pAesMacCtx)
{

#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
  (void)mbedtls_cipher_cmac_update(pAesMacCtx, pInData, InDataLength);
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
}

/**
  * @brief   StSafeA_AES_MAC_LastUpdate
  *          Update / Add data to MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pInData : data buffer
  * @param   InDataLength : data buffer length
  * @param   pAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_LastUpdate(uint8_t *pInData, uint16_t InDataLength, void *pAesMacCtx)
{
StSafeA_AES_MAC_Update(pInData, InDataLength, pAesMacCtx);
}

/**
  * @brief   StSafeA_AES_MAC_Final
  *          Finalize AES MAC computation
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pOutMac : calculated MAC
  * @param   ppAesMacCtx : AES MAC context
  * @retval  None
  */
void StSafeA_AES_MAC_Final(uint8_t *pOutMac, void **ppAesMacCtx)
{

#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
  (void)mbedtls_cipher_cmac_finish(*ppAesMacCtx, pOutMac);
  mbedtls_cipher_free(*ppAesMacCtx);
  *ppAesMacCtx = NULL;
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
}

/**
  * @brief   StSafeA_AES_ECB_Encrypt
  *          AES ECB Encryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pInData : plain data buffer
  * @param   pOutData : encrypted output data buffer
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_ECB_Encrypt(uint8_t *pInData, uint8_t *pOutData, uint8_t InAesType)
{

#ifdef MBEDTLS_AES_C
  int32_t status_code;
  mbedtls_aes_context aes;

  switch (InAesType)
  {
    case STSAFEA_KEY_TYPE_AES_128:
    case STSAFEA_KEY_TYPE_AES_256:
      mbedtls_aes_init(&aes);
      status_code = mbedtls_aes_setkey_enc(&aes, aHostCipherKey, STSAFEA_AES_KEY_BITSIZE((uint32_t)InAesType));
      if ((status_code == 0) && (pInData != NULL) && (pOutData != NULL))
      {
        status_code = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, pInData, pOutData);
      }
      mbedtls_aes_free(&aes);
      break;

    default:
      status_code = 1;
      break;
  }

  return status_code;
#else
  return 1;
#endif /* MBEDTLS_AES_C */
}

/**
  * @brief   StSafeA_AES_CBC_Encrypt
  *          AES CBC Encryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pInData : plain data buffer
  * @param   InDataLength : plain data buffer length
  * @param   pOutData : encrypted output data buffer
  * @param   InInitialValue : initial value
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_CBC_Encrypt(uint8_t *pInData, uint16_t InDataLength, uint8_t *pOutData, uint8_t *InInitialValue,
                                uint8_t InAesType)
{

#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
  int32_t status_code;
  mbedtls_aes_context aes;

  switch (InAesType)
  {
    case STSAFEA_KEY_TYPE_AES_128:
    case STSAFEA_KEY_TYPE_AES_256:
      mbedtls_aes_init(&aes);
      status_code = mbedtls_aes_setkey_enc(&aes, aHostCipherKey, STSAFEA_AES_KEY_BITSIZE((uint32_t)InAesType));
      if ((status_code == 0) && (pInData != NULL) && (pOutData != NULL) && (InInitialValue != NULL))
      {
        status_code = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, InDataLength, InInitialValue, pInData, pOutData);
      }
      mbedtls_aes_free(&aes);
      break;

    default:
      status_code = 1;
      break;
  }

  return status_code;
#else
  return 1;
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
}

/**
  * @brief   StSafeA_AES_CBC_Decrypt
  *          AES CBC Decryption
  * @note    This is a weak function that MUST be implemented at application interface level.
  *          A specific example template stsafea_crypto_xxx_interface_template.c is provided with this BSP
  *
  * @param   pInData : encrypted data buffer
  * @param   InDataLength : encrypted data buffer length
  * @param   pOutData : plain output data buffer
  * @param   InInitialValue : initial value
  * @param   InAesType : type of AES. Can be one of the following values:
  *            @arg STSAFEA_KEY_TYPE_AES_128: AES 128-bits
  *            @arg STSAFEA_KEY_TYPE_AES_256: AES 256-bits
  * @retval  0 if success, an error code otherwise
  */
int32_t StSafeA_AES_CBC_Decrypt(uint8_t *pInData, uint16_t InDataLength, uint8_t *pOutData,
                                uint8_t *InInitialValue, uint8_t InAesType)
{
#if defined MBEDTLS_AES_C & defined MBEDTLS_CIPHER_MODE_CBC
  int32_t status_code;
  mbedtls_aes_context aes;

  switch (InAesType)
  {
    case STSAFEA_KEY_TYPE_AES_128:
    case STSAFEA_KEY_TYPE_AES_256:
      mbedtls_aes_init(&aes);
      status_code = mbedtls_aes_setkey_dec(&aes, aHostCipherKey, STSAFEA_AES_KEY_BITSIZE((uint32_t)InAesType));
      if ((status_code == 0) && (pInData != NULL) && (pOutData != NULL) && (InInitialValue != NULL))
      {
        status_code = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, InDataLength, InInitialValue, pInData, pOutData);
      }
      mbedtls_aes_free(&aes);
      break;

    default:
      status_code = 1;
      break;
  }

  return status_code;
#else
  return 1;
#endif /* MBEDTLS_AES_C - MBEDTLS_CIPHER_MODE_CBC */
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

