/*
 * FreeRTOS STM32 Reference Integration
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

#include "logging_levels.h"
#define LOG_LEVEL    LOG_INFO

#include "logging.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "stdbool.h"
#include "message_buffer.h"
#include "atomic.h"

#include "eth_prv.h"

#define EVT_ETH_DONE        0x8
#define EVT_ETH_ERROR       0x10
#define ETH_EVT_DMA_IDX     1

static EthDataplaneCtx_t * volatile pxEthCtx = NULL;
static uint32_t u32_pbuf_payload_offset = 0;

extern ETH_TxPacketConfigTypeDef TxConfig;

/* Callback functions */

static void eth_transfer_done_callback( ETH_HandleTypeDef *heth )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t rslt = pdFALSE;

    if( pxEthCtx != NULL )
    {
        rslt = xTaskNotifyIndexedFromISR( pxEthCtx->xDataPlaneTaskHandle,
                                          ETH_EVT_DMA_IDX,
                                          EVT_ETH_DONE,
                                          eSetBits,
                                          &xHigherPriorityTaskWoken );
        configASSERT( rslt == pdTRUE );

        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

static void eth_transfer_error_callback( ETH_HandleTypeDef *heth )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    BaseType_t rslt = pdFALSE;

    if( pxEthCtx != NULL )
    {
        rslt = xTaskNotifyIndexedFromISR( pxEthCtx->xDataPlaneTaskHandle,
                                          ETH_EVT_DMA_IDX,
                                          EVT_ETH_ERROR,
                                          eSetBits,
                                          &xHigherPriorityTaskWoken );
        configASSERT( rslt == pdTRUE );

        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

/* Notify / means data is ready */
static void eth_notify_callback( void * pvContext )
{
    EthDataplaneCtx_t * pxCtx = ( EthDataplaneCtx_t * ) pvContext;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if( pxEthCtx != NULL )
    {
        vTaskNotifyGiveIndexedFromISR( pxCtx->xDataPlaneTaskHandle,
                                       DATA_WAITING_IDX,
                                       &xHigherPriorityTaskWoken );

        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

static inline BaseType_t xWaitForEthEvent( TickType_t xTimeout )
{
    BaseType_t xWaitResult = pdFALSE;
    BaseType_t xReturnValue = pdFALSE;
    uint32_t ulNotifiedValue = 0;

    LogDebug( "Starting wait for Eth DMA event, Timeout=%d", xTimeout );

    xWaitResult = xTaskNotifyWaitIndexed( ETH_EVT_DMA_IDX, 0, 0xFFFFFFFF, &ulNotifiedValue, xTimeout );

    if( xWaitResult == pdTRUE )
    {
        if( ulNotifiedValue & EVT_ETH_DONE )
        {
            LogDebug( "Eth done event received." );
            xReturnValue = pdTRUE;
        }

        if( ulNotifiedValue & EVT_ETH_ERROR )
        {
            LogError( "Eth error event received." );
            xReturnValue = pdFALSE;
        }

        if( ( ulNotifiedValue & ( EVT_ETH_ERROR | EVT_ETH_DONE ) ) == 0 )
        {
            LogError( "Timeout while waiting for Eth event." );
            xReturnValue = pdFALSE;
        }
    }

    return xReturnValue;
}

static inline BaseType_t xTransmitMessage( EthDataplaneCtx_t * pxCtx,
                                           uint8_t * pucTxBuffer,
                                           uint32_t usTxDataLen )
{
    HAL_StatusTypeDef xHalStatus = HAL_OK;

    configASSERT( pxCtx != NULL );
    configASSERT( pucTxBuffer != NULL );
    configASSERT( usTxDataLen > 0 );

    ( void ) xTaskNotifyStateClearIndexed( NULL, ETH_EVT_DMA_IDX );

    ETH_BufferTypeDef Txbuffer = { 0 };

    Txbuffer.buffer = pucTxBuffer;
    Txbuffer.len = usTxDataLen;

    TxConfig.Length =  usTxDataLen;
    TxConfig.TxBuffer = &Txbuffer;

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    /* NB: May only clean if the payload is cache line - aligned (32 bytes). */
/*  configASSERT( ( ((int)pucTxBuffer) & 0x1F ) == 0 ); */
    SCB_CleanDCache_by_Addr((uint32_t*)pucTxBuffer, usTxDataLen);
#endif

    if( HAL_ETH_Transmit_IT(pxCtx->pxEthHandle, &TxConfig) != HAL_OK )
    {
      while(1) { ; }
    }

    xHalStatus |= ( xWaitForEthEvent( ETH_EVENT_TIMEOUT ) == pdTRUE ) ? HAL_OK : HAL_ERROR;

    return xHalStatus == HAL_OK;
}

void vInitCallbacks( EthDataplaneCtx_t * pxCtx )
{

  HAL_StatusTypeDef xHalResult = HAL_OK;
  configASSERT( xHalResult == HAL_OK );
}

void vDataplaneThread( void * pvParameters )
{
    /* Get context struct (contains instance parameters) */
    EthDataplaneCtx_t * pxCtx = ( EthDataplaneCtx_t * ) pvParameters;

    BaseType_t exitFlag = pdFALSE;

    /* Export context for callbacks */
    pxEthCtx = pxCtx;

    vInitCallbacks( pxCtx );

    while( exitFlag == pdFALSE )
    {
        PacketBuffer_t * pxTxBuff = NULL;
        PacketBuffer_t * pxRxBuff = NULL;

        if( pxCtx->ulTxPacketsWaiting == 0 )
        {
            LogDebug( "Starting wait for DATA_WAITING_IDX event" );
            ulTaskNotifyTakeIndexed( DATA_WAITING_IDX,
                                     pdFALSE,
                                     500 );
        }

        BaseType_t xResult = pdTRUE;
        uint16_t usTxLen = 0;

        QueueHandle_t xSourceQueue = NULL;

        if( xQueuePeek( pxCtx->xDataPlaneSendQueue, &pxTxBuff, 0 ) == pdTRUE )
        {
            configASSERT( pxTxBuff != NULL );
            configASSERT( pxTxBuff->ref > 0 );
            usTxLen = pxTxBuff->tot_len;
            xSourceQueue = pxCtx->xDataPlaneSendQueue;
            LogDebug( "Preparing dataplane message for transmission" );
        }
        else
        {
            /* Empty, no TX packets */
        }

        if( ( pxTxBuff == NULL ) &&
            ( pxCtx->ulTxPacketsWaiting != 0 ) )
        {
            LogWarn( "Mismatch between ulTxPacketsWaiting and queue contents. Resetting ulTxPacketsWaiting" );
            pxCtx->ulTxPacketsWaiting = 0;
        }

        /* Read from the queue */
        if( ( xResult == pdTRUE ) &&
            ( xSourceQueue != NULL ) )
        {
            xResult = xQueueReceive( xSourceQueue, &pxTxBuff, 0 );
            configASSERT( pxTxBuff != NULL );
            configASSERT( xResult == pdTRUE );
        }
        else if( pxTxBuff != NULL )
        {
            pxTxBuff = NULL;
        }

        /* Transmit / receive packet data */
        if( xResult == pdTRUE )
        {
            /* Transmit case */
            if( usTxLen > 0 )
            {
                configASSERT( pxTxBuff );
                xResult = xTransmitMessage( pxCtx, pxTxBuff->payload, usTxLen );
            }
        }
        if( xResult == pdTRUE )
        {
            /* Check also for RX packets. It does not cost much, and saves an additional event queue.
             * The pbuf was allocated by HAL_ETH_RxAllocateCallback(). */

          ( void ) xTaskNotifyStateClearIndexed( NULL, ETH_EVT_DMA_IDX );

            if( HAL_ETH_ReadData(pxCtx->pxEthHandle, (void **) &pxRxBuff) != HAL_OK )
            {
              xResult = pdFALSE;
            }
        }

        if( pxTxBuff != NULL )
        {
            /* Decrement TX packets waiting counter */
            ( void ) Atomic_Decrement_u32( &( pxEthCtx->ulTxPacketsWaiting ) );

            /* Free the TX buffer */
            LogDebug( "Decreasing reference count of pxTxBuff %p from %d to %d", pxTxBuff, pxTxBuff->ref, ( pxTxBuff->ref - 1 ) );
            PBUF_FREE( pxTxBuff );
            pxTxBuff = NULL;
        }

        if( ( xResult == pdTRUE ) &&
            ( pxRxBuff != NULL ) )
        {
            /* Feed LwIP the received packet. */
            if( prvxLinkInput( pxCtx->pxNetif, pxRxBuff ) == pdTRUE )
            {
              pxRxBuff = NULL;
            }
        }

        if( pxRxBuff != NULL )
        {
            LogDebug( "Decreasing reference count of pxRxBuff %p from %d to %d", pxRxBuff, pxRxBuff->ref, ( pxRxBuff->ref - 1 ) );
            PBUF_FREE( pxRxBuff );
            pxRxBuff = NULL;
        }

        configASSERT( pxTxBuff == NULL );
        configASSERT( pxRxBuff == NULL );
    }
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  PacketBuffer_t * pxRxBuff = NULL;

  ( void ) pEnd;
  configASSERT( buff );

#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  /* NB: May only invalidate if the payload is cache line - aligned (32 bytes) */
  configASSERT( ( ((uint32_t)*pStart) & 0x1F ) == 0 );
  SCB_InvalidateDCache_by_Addr((uint32_t*)(*pStart), ETH_RX_BUFFER_SIZE);
#endif

  /* Compute the pbuf address from the start address of the RX payload */
  pxRxBuff = ( PacketBuffer_t *) ( (uint32_t) buff - u32_pbuf_payload_offset );

  pxRxBuff->len = Length;
  pxRxBuff->next = NULL;

  if( *pStart == NULL )
  {
    pxRxBuff->tot_len = pxRxBuff->len;
    *pStart = pxRxBuff;
  }
  else
  {
    ((PacketBuffer_t *)(*pStart))->next = pxRxBuff;
    ((PacketBuffer_t *)(*pStart))->tot_len += Length;
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t ** buff)
{
  PacketBuffer_t * pxRxBuff = NULL;
  pxRxBuff = PBUF_ALLOC_RX( ETH_RX_BUFFER_SIZE );
  if( pxRxBuff != NULL )
  {
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    /* NB: May only invalidate if the payload is cache line - aligned (32 bytes) */
    configASSERT( ( ((int)pxRxBuff->payload) & 0x1F ) == 0 );
    SCB_InvalidateDCache_by_Addr((uint32_t*)pxRxBuff->payload, ETH_RX_BUFFER_SIZE);
#endif
    *buff = pxRxBuff->payload;
    if( u32_pbuf_payload_offset == 0 )
    {
      u32_pbuf_payload_offset = ( uint32_t )( pxRxBuff->payload ) - ( uint32_t ) pxRxBuff;
    }
  }
  else
  {
    /* Rx Buffer Pool is exhausted. */
    *buff = NULL;
  }
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
  (void) heth;
  if( pxEthCtx != NULL )
  {
    eth_notify_callback( pxEthCtx );
  }
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
  eth_transfer_done_callback( heth );
}

void HAL_ETH_DMAErrorCallback(ETH_HandleTypeDef *heth)
{
  eth_transfer_error_callback( heth );
}

void HAL_ETH_MACErrorCallback(ETH_HandleTypeDef *heth)
{
  eth_transfer_error_callback( heth );
}
