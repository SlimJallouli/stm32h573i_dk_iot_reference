/*
 * AWS IoT Device SDK for Embedded C 202108.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file core_pkcs11_config.h
 * @brief PCKS#11 config options.
 */
#ifndef _CORE_PKCS11_CONFIG_H_
#define _CORE_PKCS11_CONFIG_H_

#include "main.h"
#include "FreeRTOS.h"

//#define PKCS11_PAL_LITTLEFS    1

#if (PKCS11_PAL_LITTLEFS + PKCS11_PAL_STSAFE != 1)
#error "Exactly one PKCS11_PAL flag must be set to 1."
#endif

/**
 * @brief Malloc API used by core_pkcs11.h
 */
#define pkcs11configPKCS11_MALLOC                          pvPortMalloc

/**
 * @brief Free API used by core_pkcs11.h
 */
#define pkcs11configPKCS11_FREE                            vPortFree

/**
 * @brief PKCS #11 default user PIN.
 *
 * The PKCS #11 standard specifies the presence of a user PIN. That feature is
 * sensible for applications that have an interactive user interface and memory
 * protections. However, since typical microcontroller applications lack one or
 * both of those, the user PIN is assumed to be used herein for interoperability
 * purposes only, and not as a security feature.
 *
 * Note: Do not cast this to a pointer! The library calls sizeof to get the length
 * of this string.
 */
#define pkcs11configPKCS11_DEFAULT_USER_PIN                "0000"

/**
 * @brief Maximum length (in characters) for a PKCS #11 CKA_LABEL
 * attribute.
 */
#define pkcs11configMAX_LABEL_LENGTH                       32UL

/**
 * @brief Maximum number of token objects that can be stored
 * by the PKCS #11 module.
 */
#define pkcs11configMAX_NUM_OBJECTS                        32U

/**
 * @brief Maximum number of sessions that can be stored
 * by the PKCS #11 module.
 */
#define pkcs11configMAX_SESSIONS                           10U

/**
 * @brief Set to 1 if a PAL destroy object is implemented.
 *
 * If set to 0, no PAL destroy object is implemented, and this functionality
 * is implemented in the common PKCS #11 layer.
 */
#define pkcs11configPAL_DESTROY_SUPPORTED                  1

/**
 * @brief Set to 1 if OTA image verification via PKCS #11 module is supported.
 *
 * If set to 0, OTA code signing certificate is built in via
 * aws_ota_codesigner_certificate.h.
 */
#define pkcs11configOTA_SUPPORTED                          1

/**
 * @brief Set to 1 if PAL supports storage for JITP certificate,
 * code verify certificate, and trusted server root certificate.
 *
 * If set to 0, PAL does not support storage mechanism for these, and
 * they are accessed via headers compiled into the code.
 */
#define pkcs11configJITP_CODEVERIFY_ROOT_CERT_SUPPORTED    1

/**
 * @brief The PKCS #11 label for device private key.
 *
 * Private key for connection to AWS IoT endpoint.  The corresponding
 * public key should be registered with the AWS IoT endpoint.
 */
#define pkcs11_TLS_KEY_PRV_LABEL                           "tls_key_priv"
#define pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS       ( pkcs11_TLS_KEY_PRV_LABEL )
/**
 * @brief The PKCS #11 label for device public key.
 *
 * The public key corresponding to pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS.
 */
#define pkcs11_TLS_KEY_PUB_LABEL                           "tls_key_pub"
#define pkcs11configLABEL_DEVICE_PUBLIC_KEY_FOR_TLS        ( pkcs11_TLS_KEY_PUB_LABEL )

/**
 * @brief The PKCS #11 label for the device certificate.
 *
 * Device certificate corresponding to pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS.
 */
#define pkcs11_TLS_CERT_LABEL                              "tls_cert"
#define pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS       ( pkcs11_TLS_CERT_LABEL )

/**
 * @brief The PKCS #11 label for the object to be used for HMAC operations.
 */
#define pkcs11configLABEL_HMAC_KEY                         "sntp_hmac"

/**
 * @brief The PKCS #11 label for the object to be used for CMAC operations.
 */
#define pkcs11configLABEL_CMAC_KEY                         "sntp_cmac"

/**
 * @brief The PKCS #11 label for the object to be used for code verification.
 *
 * Used by over-the-air update code to verify an incoming signed image.
 */
#define pkcs11configLABEL_CODE_VERIFICATION_KEY            "ota_signer_pub"

/**
 * @brief The PKCS #11 label for the claim certificate for Fleet Provisioning.
 */
#define pkcs11configLABEL_CLAIM_CERTIFICATE                "fleetprov_claim_cert"

/**
 * @brief The PKCS #11 label for the claim private key for Fleet Provisioning.
 */
#define pkcs11configLABEL_CLAIM_PRIVATE_KEY                "fleetprov_claim_key"

/**
 * @brief The PKCS #11 label for Just-In-Time-Provisioning.
 *
 * The certificate corresponding to the issuer of the device certificate
 * (pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS) when using the JITR or
 * JITP flow.
 */
#define pkcs11configLABEL_JITP_CERTIFICATE                 "jitp_cert"

/**
 * @brief The PKCS #11 label for the AWS Trusted Root Certificate.
 *
 * @see aws_default_root_certificates.h
 */
#define pkcs11_ROOT_CA_CERT_LABEL                          "root_ca_cert"
#define pkcs11configLABEL_ROOT_CERTIFICATE                 ( pkcs11_ROOT_CA_CERT_LABEL )

#endif /* _CORE_PKCS11_CONFIG_H_ */
