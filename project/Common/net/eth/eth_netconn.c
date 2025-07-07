/*
 * FreeRTOS STM32 Reference Integration
 *
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include "logging_levels.h"
#define LOG_LEVEL    LOG_INFO
#include "logging.h"

/* Standard includes */
#include <stdint.h>
#include <limits.h>

#include "eth_netconn.h"
#include "eth_lwip.h"
#include "eth_prv.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "main.h"

/* lwip includes */
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "lwip/prot/dhcp.h"
#include "lwip/apps/lwiperf.h"

#include "sys_evt.h"

#define MACADDR_RETRY_WAIT_TIME_TICKS    pdMS_TO_TICKS( 10 * 1000 )

static TaskHandle_t xNetTaskHandle = NULL;
static EthDataplaneCtx_t xDataPlaneCtx;

extern ETH_HandleTypeDef heth;

#if LOG_LEVEL == LOG_DEBUG

/*
 * @brief Converts from a EthStatus_t to a C string.
 */
static const char * pcEthStatusToString( EthStatus_t xStatus )
{
    const char * pcReturn = "Unknown";

    switch( xStatus )
    {
        case ETH_STATUS_NONE:
            pcReturn = "None";
            break;

        case ETH_STATUS_DOWN:
            pcReturn = "Interface Down";
            break;

        case ETH_STATUS_UP:
            pcReturn = "Interface Up";
            break;

        case ETH_STATUS_GOT_IP:
            pcReturn = "Interface Got IP";
            break;

        default:
            /* default to "Unknown" string */
            break;
    }

    return pcReturn;
}
#endif /* if LOG_LEVEL == LOG_DEBUG */

/* Wait for all bits in ulTargetBits */
static uint32_t ulWaitForNotifyBits( BaseType_t uxIndexToWaitOn,
                                     uint32_t ulTargetBits,
                                     TickType_t xTicksToWait )
{
    TickType_t xRemainingTicks = xTicksToWait;
    TimeOut_t xTimeOut;

    vTaskSetTimeOutState( &xTimeOut );

    uint32_t ulNotifyValueAccumulate = 0x0;

    while( ( ulNotifyValueAccumulate & ulTargetBits ) != ulTargetBits )
    {
        uint32_t ulNotifyValue = 0x0;
        ( void ) xTaskNotifyWaitIndexed( uxIndexToWaitOn,
                                         0x0,
                                         ulTargetBits, /* Clear only the target bits on return */
                                         &ulNotifyValue,
                                         xRemainingTicks );

        /* Accumulate notification bits */
        if( ulNotifyValue > 0 )
        {
            ulNotifyValueAccumulate |= ulNotifyValue;
        }

        /* xTaskCheckForTimeOut adjusts xRemainingTicks */
        if( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTicks ) == pdTRUE )
        {
            /* Timed out. Exit loop */
            break;
        }
    }

    /* Check for other event bits received */
    if( ( ulNotifyValueAccumulate & ( ~ulTargetBits ) ) > 0 )
    {
        /* send additional notification so these events are not lost */
        ( void ) xTaskNotifyIndexed( xTaskGetCurrentTaskHandle(),
                                     uxIndexToWaitOn,
                                     0,
                                     eNoAction );
    }

    return( ( ulTargetBits & ulNotifyValueAccumulate ) > 0 );
}

static void vHandleEthStatusUpdate( EthNetConnectCtx_t * pxCtx )
{
    if( pxCtx->xStatus != pxCtx->xStatusPrevious )
    {
        switch( pxCtx->xStatus )
        {
            case ETH_STATUS_UP:
            case ETH_STATUS_GOT_IP:
                /* Set link up */
                vSetLinkUp( &( pxCtx->xNetif ) );
                break;
            case ETH_STATUS_NONE:
            case ETH_STATUS_DOWN:
                vSetLinkDown( &( pxCtx->xNetif ) );
                break;

            default:
                LogWarn( "Unknown Ethernet link status indication: %d", pxCtx->xStatus );
                /* Fail safe to setting link up */
                vSetLinkUp( &( pxCtx->xNetif ) );
                break;
        }
    }
}

static BaseType_t xWaitForEthStatus( EthNetConnectCtx_t * pxCtx,
                                    EthStatus_t xTargetStatus,
                                    TickType_t xTicksToWait )
{
    TickType_t xRemainingTicks = xTicksToWait;
    TimeOut_t xTimeOut;

    if( pxCtx->xStatus == xTargetStatus )
    {
        return pdTRUE;
    }

    vTaskSetTimeOutState( &xTimeOut );

    uint32_t ulNotifyBits;

    while( pxCtx->xStatus <= xTargetStatus )
    {
        ulNotifyBits = ulWaitForNotifyBits( NET_EVT_IDX,
                                            ETH_STATUS_UPDATE_BIT,
                                            xRemainingTicks );

        /* xTaskCheckForTimeOut adjusts xRemainingTicks */
        if( ( ulNotifyBits > 0 ) ||
            ( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTicks ) == pdTRUE ) )
        {
            /* Timed out or success. Exit loop */
            break;
        }
    }

    return( pxCtx->xStatus >= xTargetStatus );
}

BaseType_t net_request_reconnect( void )
{
    BaseType_t xReturn = pdFALSE;

    LogDebug( "net_request_reconnect" );

    if( xNetTaskHandle != NULL )
    {
        xReturn = xTaskNotifyIndexed( xNetTaskHandle,
                                      NET_EVT_IDX,
                                      ASYNC_REQUEST_RECONNECT_BIT,
                                      eSetBits );
    }

    return xReturn;
}

/*
 * Handles network interface state change notifications from the control plane.
 */
static void vEthStatusNotify( EthStatus_t xNewStatus,
                             void * pvCtx )
{
    EthNetConnectCtx_t * pxCtx = ( EthNetConnectCtx_t * ) pvCtx;

    pxCtx->xStatusPrevious = pxCtx->xStatus;
    pxCtx->xStatus = xNewStatus;

    LogDebug( "Ethernet Status notification: %s -> %s ",
              pcEthStatusToString( pxCtx->xStatusPrevious ),
              pcEthStatusToString( pxCtx->xStatus ) );

    vHandleEthStatusUpdate( pxCtx );

    ( void ) xTaskNotifyIndexed( pxCtx->xNetTaskHandle,
                                 NET_EVT_IDX,
                                 ETH_STATUS_UPDATE_BIT,
                                 eSetBits );
}

static void vLwipReadyCallback( void * pvCtx )
{
    EthNetConnectCtx_t * pxCtx = ( EthNetConnectCtx_t * ) pvCtx;

    if( xNetTaskHandle != NULL )
    {
        ( void ) xTaskNotifyIndexed( pxCtx->xNetTaskHandle,
                                     NET_EVT_IDX,
                                     NET_LWIP_READY_BIT,
                                     eSetBits );
    }
}

static BaseType_t xNetConnect( EthNetConnectCtx_t * pxCtx )
{
  HAL_StatusTypeDef ret;

  if( ( pxCtx->xStatus == ETH_STATUS_NONE ) ||
      ( pxCtx->xStatus == ETH_STATUS_DOWN ) )
  {
    ret = HAL_ETH_Start_IT(xDataPlaneCtx.pxEthHandle);
    if( ret == HAL_OK )
    {
      vEthStatusNotify( ETH_STATUS_UP, pxCtx );
    }
  }

  return( pxCtx->xStatus >= ETH_STATUS_UP );
}

static BaseType_t xNetDisconnect( EthNetConnectCtx_t * pxCtx )
{
  HAL_StatusTypeDef ret;

  if( pxCtx->xStatus >= ETH_STATUS_UP )
  {
    ret = HAL_ETH_Stop_IT(xDataPlaneCtx.pxEthHandle);
    if( ret == HAL_OK )
    {
      vEthStatusNotify( ETH_STATUS_DOWN, pxCtx );
    }
  }

  return( pxCtx->xStatus <= ETH_STATUS_UP );
}


static void vInitializeContexts( EthNetConnectCtx_t * pxCtx )
{
    QueueHandle_t xDataPlaneSendQueue;

    /* Construct queues */
    xDataPlaneSendQueue = xQueueCreate( DATA_PLANE_QUEUE_LEN, sizeof( PacketBuffer_t * ) );
    configASSERT( xDataPlaneSendQueue != NULL );

    /* Initialize connect context */
    pxCtx->xStatus = ETH_STATUS_NONE;

    ( void ) memset( &( pxCtx->xMacAddress ), 0, sizeof( MacAddress_t ) );

    pxCtx->xDataPlaneSendQueue = xDataPlaneSendQueue;
    pxCtx->pulTxPacketsWaiting = &( xDataPlaneCtx.ulTxPacketsWaiting );
    pxCtx->xNetTaskHandle = xTaskGetCurrentTaskHandle();

    /* Construct dataplane context */
    xDataPlaneCtx.pxEthHandle = &heth;
    /* Initialize waiting packet counters */
    xDataPlaneCtx.ulTxPacketsWaiting = 0;

    /* Set queue handles */
    xDataPlaneCtx.xDataPlaneSendQueue = xDataPlaneSendQueue;
    xDataPlaneCtx.pxNetif = &( pxCtx->xNetif );
}


/*
 * Networking thread main function.
 */
void net_main( void * pvParameters )
{
    BaseType_t xResult = 0;

    EthNetConnectCtx_t xCtx;
    struct netif * pxNetif = &( xCtx.xNetif );

    /* Set static task handle var for callbacks */
    xNetTaskHandle = xTaskGetCurrentTaskHandle();

    vInitializeContexts( &xCtx );

    /* Initialize lwip */
    tcpip_init( vLwipReadyCallback, &xCtx );

    /* Wait for lwip ready callback */
    xResult = ulWaitForNotifyBits( NET_EVT_IDX,
                                   NET_LWIP_READY_BIT,
                                   portMAX_DELAY );

    /* Start dataplane thread (does hw reset on initialization) */
    xResult = xTaskCreate( &vDataplaneThread,
                           "EthData",
                           256,
                           &xDataPlaneCtx,
                           25,
                           &xDataPlaneCtx.xDataPlaneTaskHandle );

    configASSERT( xResult == pdTRUE );
    
    xCtx.xDataPlaneTaskHandle = xDataPlaneCtx.xDataPlaneTaskHandle;

    err_t xLwipError = ERR_OK;

    /* Register lwip netif */
    xLwipError = netifapi_netif_add( pxNetif,
                                     NULL, NULL, NULL,
                                     &xCtx,
                                     &prvInitNetInterface,
                                     tcpip_input );

    configASSERT( xLwipError == ERR_OK );

    netifapi_netif_set_default( pxNetif );

    netifapi_netif_set_up( pxNetif );

    ( void ) xEventGroupSetBits( xSystemEvents, EVT_MASK_NET_INIT );

    xNetConnect( &xCtx );

    if( xCtx.xStatus >= ETH_STATUS_UP )
    {
        vSetAdminUp( pxNetif );
        vStartDhcp( pxNetif );
    }

    /* Outer loop. Reinitializing */
    for( ; ; )
    {
        /* Make a connection attempt */
        if( ( xCtx.xStatus != ETH_STATUS_UP ) &&
            ( xCtx.xStatus != ETH_STATUS_GOT_IP ) )
        {
            xNetConnect( &xCtx );
        }

        /*
         * Wait for any event or timeout after 30 seconds
         */
        uint32_t ulNotificationValue = 0x0;
        xResult = xTaskNotifyWaitIndexed( NET_EVT_IDX,
                                          0x0,
                                          0xFFFFFFFF,
                                          &ulNotificationValue,
                                          pdMS_TO_TICKS( 30 * 1000 ) );

        if( ulNotificationValue != 0 )
        {
            /* Latch in current flags */
            uint8_t ucNetifFlags = pxNetif->flags;

            /* Handle state changes from the driver */
            if( ( ulNotificationValue & ETH_STATUS_UPDATE_BIT ) )
            {
                vHandleEthStatusUpdate( &xCtx );
            }

            if( ulNotificationValue & NET_LWIP_IP_CHANGE_BIT )
            {
                LogSys( "IP Address Change." );
                vLogAddress( "IP Address:", pxNetif->ip_addr );
                vLogAddress( "Gateway:", pxNetif->gw );
                vLogAddress( "Netmask:", pxNetif->netmask );

                ( void ) xEventGroupSetBits( xSystemEvents, EVT_MASK_NET_CONNECTED );
            }

            if( ulNotificationValue & NET_LWIP_IFUP_BIT )
            {
                LogInfo( "Administrative UP event." );

                vStartDhcp( pxNetif );
            }
            else if( ( ulNotificationValue & NET_LWIP_LINK_UP_BIT ) &&
                     ( ucNetifFlags & NETIF_FLAG_LINK_UP ) )
            {
                LogInfo( "Link UP event." );

                vSetAdminUp( pxNetif );
                vStartDhcp( pxNetif );
                LogSys( "Network Link Up." );
            }
            else if( ulNotificationValue & NET_LWIP_IFDOWN_BIT )
            {
                LogInfo( "Administrative DOWN event." );

                vStopDhcp( pxNetif );
                vClearAddress( pxNetif );
                ( void ) xEventGroupClearBits( xSystemEvents, EVT_MASK_NET_CONNECTED );
            }
            else if( ( ulNotificationValue & NET_LWIP_LINK_DOWN_BIT ) &&
                     ( ( ucNetifFlags & NETIF_FLAG_LINK_UP ) == 0 ) )
            {
                vSetAdminDown( pxNetif );
                vStopDhcp( pxNetif );
                vClearAddress( pxNetif );
                LogSys( "Network Link Down." );
                ( void ) xEventGroupClearBits( xSystemEvents, EVT_MASK_NET_CONNECTED );
            }

            /* Reconnect requested by configStore or cli process */
            if( ulNotificationValue & ASYNC_REQUEST_RECONNECT_BIT )
            {
                ( void ) xEventGroupClearBits( xSystemEvents, EVT_MASK_NET_CONNECTED );
                xNetDisconnect( &xCtx );
                xNetConnect( &xCtx );
            }
        }
    }
}

