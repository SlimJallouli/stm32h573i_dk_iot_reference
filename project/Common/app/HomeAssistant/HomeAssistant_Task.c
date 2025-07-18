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

/* MQTT library includes. */
#include "core_mqtt.h"

/* MQTT agent include. */
#include "core_mqtt_agent.h"

/* MQTT agent task API. */
#include "mqtt_agent_task.h"

#include "kvstore.h"

#include "ota_appversion32.h"

# define MAXT_TOPIC_LENGTH 128
static char publish_topic[MAXT_TOPIC_LENGTH];

#define MQTT_PUBLISH_TIME_BETWEEN_MS         ( 300 )
/**
 * @brief The maximum amount of time in milliseconds to wait for the commands
 * to be posted to the MQTT agent should the MQTT agent's command queue be full.
 * Tasks wait in the Blocked state, so don't use any CPU time.
 */
#define configMAX_COMMAND_SEND_BLOCK_TIME_MS         ( 500 )

/**
 * @brief Size of statically allocated buffers for holding payloads.
 */
#define configPAYLOAD_BUFFER_LENGTH                  ( 1024 )

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
static MQTTStatus_t prvPublishToTopic(MQTTQoS_t xQoS, bool xRetain, char *pcTopic, uint8_t *pucPayload, size_t xPayloadLength);

/**
 * @brief The function that implements the task demonstrated by this file.
 *
 * @param pvParameters The parameters to the task.
 */
void vHAConfigPublishTask(void *pvParameters);

/**
 * @brief Publishes an empty retained message to the given topic to clear the retained message.
 *
 * This function uses the MQTT agent's publish flow to remove a previously retained message
 * on a topic by publishing a zero-length payload with the retain flag set.
 *
 * @param[in] pcTopic The topic from which to clear the retained message.
 *
 * @return MQTTSuccess if the empty publish succeeded, appropriate MQTTStatus_t error otherwise.
 */
static MQTTStatus_t prvClearRetainedTopic(char *pcTopic);

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

static MQTTStatus_t prvPublishToTopic(MQTTQoS_t xQoS, bool xRetain, char *pcTopic, uint8_t *pucPayload, size_t xPayloadLength)
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
  xPublishInfo.retain = xRetain;
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
        if (ulNotifiedValue)
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

static MQTTStatus_t prvClearRetainedTopic(char *pcTopic)
{
  configASSERT(pcTopic != NULL);
  LogInfo(("Clearing retained message on topic: %s", pcTopic));
  return prvPublishToTopic(MQTTQoS0, pdTRUE, pcTopic, NULL, 0);
}

/*-----------------------------------------------------------*/

static MQTTStatus_t publishFirmwareVersionStatus(const AppVersion32_t appFirmwareVersion, const char * pcThingName)
{
    char cPayloadBuf[128];
    char cTopicBuf[64];
    int msgLen = 0;
    MQTTQoS_t xQoS = MQTTQoS0;
    bool xRetain = pdTRUE;
    MQTTStatus_t xStatus = MQTTBadParameter;

    // Compose topic: <ThingName>/fw/state
    msgLen = snprintf(cTopicBuf, sizeof(cTopicBuf), "%s/fw/state", pcThingName);
    if (msgLen < 0 || msgLen >= sizeof(cTopicBuf))
    {
        return MQTTBadParameter;
    }
    // Compose JSON payload
    msgLen = snprintf(cPayloadBuf, sizeof(cPayloadBuf),
                      "{\"installed_version\": \"%u.%u.%u\", \"latest_version\": \"%u.%u.%u\"}",
                      appFirmwareVersion.u.x.major,
                      appFirmwareVersion.u.x.minor,
                      appFirmwareVersion.u.x.build,
                      appFirmwareVersion.u.x.major,
                      appFirmwareVersion.u.x.minor,
                      appFirmwareVersion.u.x.build);

    if (msgLen < 0 || msgLen >= sizeof(cPayloadBuf))
    {
        return MQTTBadParameter;
    }

    // Publish over MQTT using QoS 0 and retain = true
    xStatus = prvPublishToTopic(xQoS, xRetain, cTopicBuf, (uint8_t*) cPayloadBuf, msgLen);

    return xStatus;
}

/*-----------------------------------------------------------*/

void vHAConfigPublishTask(void *pvParameters)
{
  char *cPayloadBuf = NULL;
  size_t xPayloadLength = 0;
  MQTTQoS_t xQoS = MQTTQoS0;
  bool xRetain = pdTRUE;
  char *pThingName = NULL;
  size_t uxThingNameLen = 0;
  char * fwVersionStr = (char*) pvPortMalloc(17);

  vSleepUntilMQTTAgentReady();
  xMQTTAgentHandle = xGetMqttAgentHandle();
  configASSERT(xMQTTAgentHandle != NULL);
  vSleepUntilMQTTAgentConnected();

  pThingName = KVStore_getStringHeap(CS_CORE_THING_NAME, &uxThingNameLen);
  configASSERT(pThingName != NULL);

  cPayloadBuf = (char*) pvPortMalloc(configPAYLOAD_BUFFER_LENGTH);
  configASSERT(cPayloadBuf != NULL);

  LogInfo(("Publishing Home Assistant discovery configuration for device: %s", pThingName));


  memset(fwVersionStr, 0, 17);

  snprintf(fwVersionStr, 16, "%d.%d.%d",
           appFirmwareVersion.u.x.major,
           appFirmwareVersion.u.x.minor,
           appFirmwareVersion.u.x.build);

#if (DEMO_OTA == 1)
  snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/update/%s_fw/config", pThingName);

  xPayloadLength = snprintf(cPayloadBuf, configPAYLOAD_BUFFER_LENGTH, "{"
      "\"name\": \"Firmware\","
      "\"unique_id\": \"%s_fw_update\","
      "\"state_topic\": \"%s/fw/state\","
      "\"value_template\": \"{{ value_json.installed_version }}\","
      "\"latest_version_topic\": \"%s/fw/state\","
      "\"latest_version_template\": \"{{ value_json.latest_version }}\","
      "\"command_topic\": \"%s/fw/update\","
      "\"payload_install\": \"start_update\","
      "\"retain\": false,"
      "\"device_class\": \"firmware\","
      "\"device\": {"
      "\"identifiers\": [\"%s\"],"
      "\"manufacturer\": \"STMicroelectronics\","
      "\"model\": \"%s\","
      "\"name\": \"%s\","
      "\"sw_version\": \"%s\""
      "}"
      "}", pThingName, pThingName, pThingName, pThingName, pThingName, BOARD, pThingName, fwVersionStr);

  if (xPayloadLength < configPAYLOAD_BUFFER_LENGTH)
  {
    prvPublishToTopic(xQoS, xRetain, configPUBLISH_TOPIC, (uint8_t*) cPayloadBuf, xPayloadLength);
  }
  else
  {
    LogError(("Firmware update payload truncated"));
  }

  vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
#endif

#if (DEMO_LED == 1)
  snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/switch/%s_led/config", pThingName);

  xPayloadLength = snprintf(cPayloadBuf, configPAYLOAD_BUFFER_LENGTH, "{"
      "\"name\": \"LED\","
      "\"unique_id\": \"%s_led\","
      "\"command_topic\": \"%s/led/desired\","
      "\"state_topic\": \"%s/led/reported\","
      "\"value_template\": \"{{ value_json.ledStatus.reported }}\","
      "\"payload_on\": \"ON\","
      "\"payload_off\": \"OFF\","
      "\"state_on\": \"ON\","
      "\"state_off\": \"OFF\","
      "\"retain\": true,"
      "\"device\": {"
      "\"identifiers\": [\"%s\"],"
      "\"manufacturer\": \"STMicroelectronics\","
      "\"model\": \"%s\","
      "\"name\": \"%s\","
      "\"sw_version\": \"%s\""
      "}"
      "}", pThingName, pThingName, pThingName, pThingName, BOARD, pThingName, fwVersionStr);

  if (xPayloadLength < configPAYLOAD_BUFFER_LENGTH)
  {
    prvPublishToTopic(xQoS, xRetain, configPUBLISH_TOPIC, (uint8_t*) cPayloadBuf, xPayloadLength);
  }
  else
  {
    LogError(("LED payload truncated"));
  }

  vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
#else
  snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/switch/%s_led/config", pThingName);

  prvClearRetainedTopic(configPUBLISH_TOPIC);

  vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
#endif

#if (DEMO_BUTTON == 1)
  snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/binary_sensor/%s_button/config", pThingName);

  xPayloadLength = snprintf(cPayloadBuf, configPAYLOAD_BUFFER_LENGTH, "{"
      "\"name\": \"Button\","
      "\"unique_id\": \"%s_button\","
      "\"state_topic\": \"%s/sensor/button/reported\","
      "\"value_template\": \"{{ value_json.buttonStatus.reported }}\","
      "\"payload_on\": \"ON\","
      "\"payload_off\": \"OFF\","
      "\"device_class\": \"occupancy\","
      "\"retain\": true,"
      "\"device\": {"
      "\"identifiers\": [\"%s\"],"
      "\"manufacturer\": \"STMicroelectronics\","
      "\"model\": \"%s\","
      "\"name\": \"%s\","
      "\"sw_version\": \"%s\""
      "}"
      "}", pThingName, pThingName, pThingName, BOARD, pThingName, fwVersionStr);

  if (xPayloadLength < configPAYLOAD_BUFFER_LENGTH)
  {
    prvPublishToTopic(xQoS, xRetain, configPUBLISH_TOPIC, (uint8_t*) cPayloadBuf, xPayloadLength);
  }
  else
  {
    LogError(("Button payload truncated"));
  }

  vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
#else
  snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/binary_sensor/%s_button/config", pThingName);

  prvClearRetainedTopic(configPUBLISH_TOPIC);

  vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
#endif

#if (DEMO_ENV_SENSOR == 1)
  const char *env_fields[] = { "temp_0_c", "temp_1_c", "rh_pct", "baro_mbar" };
  const char *env_names[] = { "Temperature 0", "Temperature 1", "Humidity", "Pressure" };
  const char *env_units[] = { "°C", "°C", "%", "mbar" };
  const char *env_classes[] = { "temperature", "temperature", "humidity", "pressure" };

  for (int i = 0; i < 4; i++)
  {
    snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/sensor/%s_%s/config", pThingName, env_fields[i]);

    xPayloadLength = snprintf(cPayloadBuf, configPAYLOAD_BUFFER_LENGTH, "{"
        "\"name\": \"%s\","
        "\"unique_id\": \"%s_env_%d\","
        "\"state_topic\": \"%s/sensor/env\","
        "\"value_template\": \"{{ value_json.%s }}\","
        "\"unit_of_measurement\": \"%s\","
        "\"device_class\": \"%s\","
        "\"retain\": true,"
        "\"device\": {"
        "\"identifiers\": [\"%s\"],"
        "\"manufacturer\": \"STMicroelectronics\","
        "\"model\": \"%s\","
        "\"name\": \"%s\","
        "\"sw_version\": \"%s\""
        "}"
        "}", env_names[i], pThingName, i, pThingName, env_fields[i], env_units[i], env_classes[i], pThingName, BOARD, pThingName, fwVersionStr);

    if (xPayloadLength < configPAYLOAD_BUFFER_LENGTH)
    {
      prvPublishToTopic(xQoS, xRetain, configPUBLISH_TOPIC, (uint8_t*) cPayloadBuf, xPayloadLength);
    }
    else
    {
      LogError(("Env sensor %d payload truncated", i));
    }

    vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
  }
#else
  const char *env_fields[] = { "temp_0_c", "temp_1_c", "rh_pct", "baro_mbar" };

  for (int i = 0; i < 4; i++)
  {
    snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/sensor/%s_%s/config", pThingName, env_fields[i]);

    prvClearRetainedTopic(configPUBLISH_TOPIC);

    vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
  }
#endif

#if (DEMO_MOTION_SENSOR == 1)
  const char *motion_roots[] = { "acceleration_mG", "gyro_mDPS", "magnetometer_mGauss" };
  const char *motion_labels[] = { "Acceleration", "Gyroscope", "Magnetometer" };
  const char *axes[] = { "x", "y", "z" };

  for (int m = 0; m < 3; m++)
  {
    for (int a = 0; a < 3; a++)
    {
      snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/sensor/%s_%s_%s/config", pThingName, motion_roots[m], axes[a]);

      if (xRetain)
      {
        xPayloadLength = snprintf(cPayloadBuf, configPAYLOAD_BUFFER_LENGTH, "{"
            "\"name\": \"%s %s\","
            "\"unique_id\": \"%s_%s_%s\","
            "\"state_topic\": \"%s/sensor/motion\","
            "\"value_template\": \"{{ value_json.%s.%s }}\","
            "\"unit_of_measurement\": \"%s\","
            "\"retain\": true,"
            "\"device\": {"
            "\"identifiers\": [\"%s\"],"
            "\"manufacturer\": \"STMicroelectronics\","
            "\"model\": \"%s\","
            "\"name\": \"%s\","
            "\"sw_version\": \"%s\""
            "}"
            "}", motion_labels[m], axes[a], pThingName, motion_roots[m], axes[a], pThingName, motion_roots[m], axes[a], (
            m == 0 ? "mG" : (m == 1 ? "mDPS" : "mG")), pThingName, BOARD, pThingName, fwVersionStr);

        if (xPayloadLength < configPAYLOAD_BUFFER_LENGTH)
        {
          prvPublishToTopic(xQoS, xRetain, configPUBLISH_TOPIC, (uint8_t*) cPayloadBuf, xPayloadLength);
        }
        else
        {
          LogError(("Motion sensor %s %s payload truncated", motion_labels[m], axes[a]));
        }
      }
      else
      {
        prvClearRetainedTopic(configPUBLISH_TOPIC);
      }

      vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
    }
  }
#else
  const char *motion_roots[] = { "acceleration_mG", "gyro_mDPS", "magnetometer_mGauss" };
  const char *axes[] = { "x", "y", "z" };

  for (int m = 0; m < 3; m++)
  {
    for (int a = 0; a < 3; a++)
    {
      snprintf(configPUBLISH_TOPIC, MAXT_TOPIC_LENGTH, "homeassistant/sensor/%s_%s_%s/config", pThingName, motion_roots[m], axes[a]);

      prvClearRetainedTopic(configPUBLISH_TOPIC);

      vTaskDelay(MQTT_PUBLISH_TIME_BETWEEN_MS);
    }
  }
#endif

  vPortFree(cPayloadBuf);
  vPortFree(pThingName);
  LogInfo("Discovery config task completed. Deleting itself.");
  vTaskDelete(NULL);
}
