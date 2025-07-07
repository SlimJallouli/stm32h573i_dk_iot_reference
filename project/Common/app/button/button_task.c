/*
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
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 */

/**
 * @brief A test application which loops through subscribing to a topic and publishing message
 * to a topic. This test application can be used with AWS IoT device advisor test suite to
 * verify that an application implemented using MQTT agent follows best practices in connecting
 * to AWS IoT core.
 */
/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"

/* MQTT agent task API. */
#include "mqtt_agent_task.h"

#include "kvstore.h"

#include "interrupt_handlers.h"

# define MAXT_TOPIC_LENGTH 64
static char publish_topic[MAXT_TOPIC_LENGTH];
#define BUTTON_RISING_EVENT    (1 << 0)
#define BUTTON_FALLING_EVENT   (1 << 1)

static EventGroupHandle_t xButtonEventGroup = NULL;
/**
 * @brief The maximum amount of time in milliseconds to wait for the commands
 * to be posted to the MQTT agent should the MQTT agent's command queue be full.
 * Tasks wait in the Blocked state, so don't use any CPU time.
 */
#define configMAX_COMMAND_SEND_BLOCK_TIME_MS         ( 500 )

/**
 * @brief Size of statically allocated buffers for holding payloads.
 */
#define configPAYLOAD_BUFFER_LENGTH                  ( 512 )

/**
 * @brief Format of topic used to publish outgoing messages.
 */
#define configPUBLISH_TOPIC                   publish_topic

/**
 * @brief Format of topic used to subscribe to incoming messages.
 *
 */
#define configSUBSCRIBE_TOPIC_FORMAT   configPUBLISH_TOPIC_FORMAT

/*-----------------------------------------------------------*/

/**
 * @brief Defines the structure to use as the command callback context in this
 * demo.
 */
struct MQTTAgentCommandContext
{
  TaskHandle_t xTaskToNotify;
  void *pArgs;
};

/*-----------------------------------------------------------*/

static MQTTAgentHandle_t xMQTTAgentHandle = NULL;

/*-----------------------------------------------------------*/

/**
 * @brief Passed into MQTTAgent_Publish() as the callback to execute when the
 * broker ACKs the PUBLISH message.  Its implementation sends a notification
 * to the task that called MQTTAgent_Publish() to let the task know the
 * PUBLISH operation completed.  It also sets the xReturnStatus of the
 * structure passed in as the command's context to the value of the
 * xReturnStatus parameter - which enables the task to check the status of the
 * operation.
 *
 * See https://freertos.org/mqtt/mqtt-agent-demo.html#example_mqtt_api_call
 *
 * @param[in] pxCommandContext Context of the initial command.
 * @param[in].xReturnStatus The result of the command.
 */
static void prvPublishCommandCallback(MQTTAgentCommandContext_t *pxCommandContext, MQTTAgentReturnInfo_t *pxReturnInfo);

/**
 * @brief Publishes the given payload using the given qos to the topic provided.
 *
 * Function queues a publish command with the MQTT agent and waits for response. For
 * Qos0 publishes command is successful when the message is sent out of network. For Qos1
 * publishes, the command succeeds once a puback is received. If publish is unsuccessful, the function
 * retries the publish for a configure number of tries.
 *
 * @param[in] xQoS The quality of service (QoS) to use.  Can be zero or one
 * for all MQTT brokers.  Can also be QoS2 if supported by the broker.  AWS IoT
 * does not support QoS2.
 * @param[in] pcTopic NULL terminated topic string to which message is published.
 * @param[in] pucPayload The payload blob to be published.
 * @param[in] xPayloadLength Length of the payload blob to be published.
 */
static MQTTStatus_t prvPublishToTopic(MQTTQoS_t xQoS, char *pcTopic, uint8_t *pucPayload, size_t xPayloadLength);

/**
 * @brief Callback function for GPIO rising edge interrupt on the user button.
 *
 * This function is registered with the EXTI driver and is triggered on a rising edge
 * (button released if active-low). It sets the corresponding event bit in the
 * xButtonEventGroup from ISR context.
 *
 * @param[in] pvContext Optional user context pointer passed during callback registration. Not used.
 */
static void user_button_rising_event(void *pvContext);

/**
 * @brief Callback function for GPIO falling edge interrupt on the user button.
 *
 * This function is registered with the EXTI driver and is triggered on a falling edge
 * (button pressed if active-low). It sets the appropriate event bit in the
 * xButtonEventGroup from ISR context.
 *
 * @param[in] pvContext Optional user context pointer passed during callback registration. Not used.
 */
static void user_button_falling_event(void *pvContext);


/**
 * @brief The function that implements the task demonstrated by this file.
 *
 * @param pvParameters The parameters to the task.
 */
void vButtonTask(void *pvParameters);

/*-----------------------------------------------------------*/

/**
 * @brief The MQTT agent manages the MQTT contexts.  This set the handle to the
 * context used by this demo.
 */
extern MQTTAgentContext_t xGlobalMqttAgentContext;

/*-----------------------------------------------------------*/

static void prvPublishCommandCallback(MQTTAgentCommandContext_t *pxCommandContext, MQTTAgentReturnInfo_t *pxReturnInfo)
{
  if (pxCommandContext->xTaskToNotify != NULL)
  {
    xTaskNotify(pxCommandContext->xTaskToNotify, pxReturnInfo->returnCode, eSetValueWithOverwrite);
  }
}

/*-----------------------------------------------------------*/

static MQTTStatus_t prvPublishToTopic(MQTTQoS_t xQoS, char *pcTopic, uint8_t *pucPayload, size_t xPayloadLength)
{
  MQTTPublishInfo_t xPublishInfo = { 0UL };
  MQTTAgentCommandContext_t xCommandContext = { 0 };
  MQTTStatus_t xMQTTStatus;
  BaseType_t xNotifyStatus;
  MQTTAgentCommandInfo_t xCommandParams = { 0UL };
  uint32_t ulNotifiedValue = 0U;

  /* Create a unique number of the subscribe that is about to be sent.  The number
   * is used as the command context and is sent back to this task as a notification
   * in the callback that executed upon receipt of the subscription acknowledgment.
   * That way this task can match an acknowledgment to a subscription. */
  xTaskNotifyStateClear(NULL);

  /* Configure the publish operation. */
  xPublishInfo.qos = xQoS;
  xPublishInfo.retain = pdFALSE;
  xPublishInfo.pTopicName = pcTopic;
  xPublishInfo.topicNameLength = (uint16_t) strlen(pcTopic);
  xPublishInfo.pPayload = pucPayload;
  xPublishInfo.payloadLength = xPayloadLength;

  xCommandContext.xTaskToNotify = xTaskGetCurrentTaskHandle();

  xCommandParams.blockTimeMs = configMAX_COMMAND_SEND_BLOCK_TIME_MS;
  xCommandParams.cmdCompleteCallback = prvPublishCommandCallback;
  xCommandParams.pCmdCompleteCallbackContext = &xCommandContext;

  /* Loop in case the queue used to communicate with the MQTT agent is full and
   * attempts to post to it time out.  The queue will not become full if the
   * priority of the MQTT agent task is higher than the priority of the task
   * calling this function. */
  do
  {
    xMQTTStatus = MQTTAgent_Publish(xMQTTAgentHandle, &xPublishInfo, &xCommandParams);

    if (xMQTTStatus == MQTTSuccess)
    {
      /* Wait for this task to get notified, passing out the value it gets  notified with. */
      xNotifyStatus = xTaskNotifyWait(0, 0, &ulNotifiedValue, portMAX_DELAY);

      if (xNotifyStatus == pdTRUE)
      {
        if(ulNotifiedValue)
        {
          xMQTTStatus = MQTTSendFailed;
        }
        else
        {
          xMQTTStatus = MQTTSuccess;
        }
      }
      else
      {
        xMQTTStatus = MQTTSendFailed;
      }
    }
  }
  while (xMQTTStatus != MQTTSuccess);

  return xMQTTStatus;
}

/*-----------------------------------------------------------*/

static void user_button_rising_event(void *pvContext)
{
    (void) pvContext;
    if (xButtonEventGroup != NULL)
    {
        xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_RISING_EVENT, NULL);
    }
}

/*-----------------------------------------------------------*/

static void user_button_falling_event(void *pvContext)
{
    (void) pvContext;
    if (xButtonEventGroup != NULL)
    {
        xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_FALLING_EVENT, NULL);
    }
}

/*-----------------------------------------------------------*/

void vButtonTask(void *pvParameters)
{
    char *cPayloadBuf;
    size_t xPayloadLength;
    MQTTStatus_t xMQTTStatus;
    MQTTQoS_t xQoS = MQTTQoS0;
    char *pThingName = NULL;
    size_t uxTempSize = 0;
    EventBits_t uxBits;

    (void) pvParameters;

    vSleepUntilMQTTAgentReady();
    xMQTTAgentHandle = xGetMqttAgentHandle();
    configASSERT(xMQTTAgentHandle != NULL);
    vSleepUntilMQTTAgentConnected();

    LogInfo(("MQTT Agent connected. Starting button reporting task."));

    pThingName = KVStore_getStringHeap(CS_CORE_THING_NAME, &uxTempSize);
    configASSERT(pThingName != NULL);

    cPayloadBuf = (char *) pvPortMalloc(configPAYLOAD_BUFFER_LENGTH);
    configASSERT(cPayloadBuf != NULL);

    xButtonEventGroup = xEventGroupCreate();
    configASSERT(xButtonEventGroup != NULL);

    snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "%s/sensor/button/reported", pThingName);

    // Register GPIO callbacks
    GPIO_EXTI_Register_Rising_Callback (USER_Button_Pin, user_button_rising_event, NULL);
    GPIO_EXTI_Register_Falling_Callback(USER_Button_Pin, user_button_falling_event, NULL);

    for (;;)
    {
        uxBits = xEventGroupWaitBits(xButtonEventGroup,
                                     BUTTON_RISING_EVENT | BUTTON_FALLING_EVENT,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);

        const char *status = (uxBits & BUTTON_RISING_EVENT) ? "ON" :
                             (uxBits & BUTTON_FALLING_EVENT) ? "OFF" : "UNKNOWN";

        xPayloadLength = snprintf(
            cPayloadBuf,
            configPAYLOAD_BUFFER_LENGTH,
            "{ \"buttonStatus\": { \"reported\": \"%s\" } }",
            status
        );

        configASSERT(xPayloadLength <= configPAYLOAD_BUFFER_LENGTH);

        LogInfo(("Publishing button status to: %s, message: %.*s",
                 configPUBLISH_TOPIC, xPayloadLength, cPayloadBuf));

        xMQTTStatus = prvPublishToTopic(xQoS, configPUBLISH_TOPIC,
                                        (uint8_t *) cPayloadBuf, xPayloadLength);

        if (xMQTTStatus == MQTTSuccess)
        {
            LogInfo(("Successfully published button status: %s", status));
        }
        else
        {
            LogError(("Failed to publish button status to topic: %s", configPUBLISH_TOPIC));
        }
    }

    // Clean up (only reached if loop is broken, not expected)
    vPortFree(pThingName);
    vPortFree(cPayloadBuf);
    vEventGroupDelete(xButtonEventGroup);
    vTaskDelete(NULL);
}
