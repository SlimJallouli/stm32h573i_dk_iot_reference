/*
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Portions:
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
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
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 */
#ifndef _ETH_PRV_
#define _ETH_PRV_

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C"
{
#endif
/* *INDENT-ON* */

/* Private definitions to be shared between Eth driver files */
#include "task.h"
#include "semphr.h"

#define LWIP_STACK

#ifdef LWIP_STACK
#include "eth_lwip.h"
#endif

#define DATA_WAITING_IDX                 3

#define NET_EVT_IDX                      0x1
#define NET_LWIP_READY_BIT               0x1
#define NET_LWIP_IP_CHANGE_BIT           0x2
#define NET_LWIP_IFUP_BIT                0x4
#define NET_LWIP_IFDOWN_BIT              0x8
#define NET_LWIP_LINK_UP_BIT             0x10
#define NET_LWIP_LINK_DOWN_BIT           0x20
#define ETH_STATUS_UPDATE_BIT            0x40
#define ASYNC_REQUEST_RECONNECT_BIT      0x80

/* Constants */
#define ETH_TIMEOUT_CONNECT              pdMS_TO_TICKS( 120 * 1000 )
#define ETH_MACADDR_LEN                  6
#define ETH_IP_LEN                       16
#define ETH_PACKET_ENQUEUE_TIMEOUT       pdMS_TO_TICKS( 5 * 10000 )
#define ETH_EVENT_TIMEOUT                pdMS_TO_TICKS( 10000 )

#define DATA_PLANE_QUEUE_LEN             10


typedef enum
{
    ETH_STATUS_NONE = 0,
    ETH_STATUS_DOWN,
    ETH_STATUS_UP,
    ETH_STATUS_GOT_IP
} EthStatus_t;

typedef struct
{
    ETH_HandleTypeDef * pxEthHandle;
    TaskHandle_t xDataPlaneTaskHandle;
    volatile uint32_t ulTxPacketsWaiting;
    volatile uint32_t ulLastRequestId;
    NetInterface_t * pxNetif;
    QueueHandle_t xDataPlaneSendQueue;
} EthDataplaneCtx_t;

typedef struct
{
    MacAddress_t xMacAddress;
    NetInterface_t xNetif;
    volatile EthStatus_t xStatus;
    volatile EthStatus_t xStatusPrevious;
    QueueHandle_t xDataPlaneSendQueue;
    volatile uint32_t * pulTxPacketsWaiting;
    TaskHandle_t xNetTaskHandle;
    TaskHandle_t xDataPlaneTaskHandle;
} EthNetConnectCtx_t;

/* IPC_WIFI_CONNECT */
typedef struct IPInfoType
{
    char cLocalIP[ ETH_IP_LEN ];
    char cNetmask[ ETH_IP_LEN ];
    char cGateway[ ETH_IP_LEN ];
    char cDnsServer[ ETH_IP_LEN ];
} IPInfoType_t;


/* IPC_WIFI_DISCONNECT */
/* IPC_WIFI_EVT_STATUS */
struct IPCResponseStatus
{
    uint32_t status;
};

typedef struct IPCResponseStatus IPCResponseWifiDisconnect_t;

/* IPC_WIFI_BYPASS_SET */

typedef struct IPCRequestWifiBypassSet
{
    int32_t enable;
} IPCRequestWifiBypassSet_t;

typedef IPCRequestWifiBypassSet_t IPCRequestWifiBypassGet_t;

/* IPC_WIFI_BYPASS_OUT */
typedef struct IPCResponseStatus  IPCResponsBypassOut_t;

/* IPC_WIFI_EVT_STATUS */
typedef struct IPCResponseStatus  IPCEventStatus_t;

/* Struct representing packet header (without SPI-specific header) */
typedef struct
{
    uint32_t ulIPCRequestId;
    uint16_t usIPCApiId; /* IPCCommand_t */
} IPCHeader_t;

#define MX_BYPASS_PAD_LEN    16

/* IPC_WIFI_BYPASS_OUT */
/* IPC_WIFI_EVT_BYPASS_IN */
typedef struct
{
    IPCHeader_t xHeader;
    int32_t lIndex; /* WifiBypassMode_t */
    uint8_t ucPad[ MX_BYPASS_PAD_LEN ];
    uint16_t usDataLen;
} BypassInOut_t;

#define ETH_MAX_MTU       1500

/*
 * static inline void vPrintBuffer( const char * ucPrefix,
 *                               uint8_t * pcBuffer,
 *                               uint32_t usDataLen )
 * {
 *  char * ucPrintBuf = pvPortMalloc( 2 * usDataLen + 1 );
 *  if( ucPrintBuf != NULL )
 *  {
 *      for( uint32_t i = 0; i < usDataLen; i++ )
 *      {
 *          snprintf( &ucPrintBuf[ 2 * i ], 3, "%02X", pcBuffer[i] );
 *      }
 *      ucPrintBuf[ 2 * usDataLen ] = 0;
 *      LogError("%s: %s", ucPrefix, ucPrintBuf);
 *      vPortFree(ucPrintBuf);
 *  }
 * }
 *
 * static inline void vPrintSpiHeader( const char * ucPrefix,
 *                                  SPIHeader_t * pxHdr )
 * {
 *  LogError("%s: Type: 0x%02X, Len: 0x%04X, Lenx: 0x%04X", ucPrefix, pxHdr->type, pxHdr->len, pxHdr->lenx );
 * }
 *
 */


BaseType_t prvxLinkInput( NetInterface_t * pxNetif,
                          PacketBuffer_t * pxPbufIn );
void prvControlPlaneRouter( void * pvParameters );
uint32_t prvGetNextRequestID( void );
void vDataplaneThread( void * pvParameters );
void low_level_init( NetInterface_t *netif );

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* _ETH_PRV_ */
