/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : FreeRTOS applicative file
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
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */
#if defined(LOG_LEVEL)
#undef LOG_LEVEL
#endif

#define LOG_LEVEL    LOG_INFO

#include "logging.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>
#if defined(LFS_CONFIG)
#include "lfs.h"
#include "lfs_port.h"
#endif
#include "kvstore.h"
#include "sys_evt.h"

#if defined(__USE_STSAFE__)
#include "stsafe.h"
#endif

#if defined(MXCHIP)
#include "mx_netconn.h"
#endif

#if defined(ST67W6X_NCP)
#include "w6x_wifi_netconn.h"
#endif

#if defined(ETHERNET)
#include "eth_netconn.h"
#endif

#if MQTT_ENABLED
#include "mqtt_agent_task.h"
#endif

#if DEMO_ECHO_SERVER
#include "echo_server.h"
#endif

#if DEMO_ECHO_CLIENT
#include "echo_client.h"
#endif

#if (DEMO_PING && !defined(ST67W6X_NCP))
#include "ping.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
EventGroupHandle_t xSystemEvents = NULL;
#if defined(LFS_CONFIG)
static lfs_t *pxLfsCtx = NULL;
#endif
/* USER CODE END Variables */


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void vInitTask(void *pvArgs);
#if defined(LFS_CONFIG)
static int fs_init(void);
lfs_t* pxGetDefaultFsCtx(void);
#endif

extern void otaPal_EarlyInit(void);

extern void vSubscribePublishTestTask    ( void * pvParameters );
extern void vDefenderAgentTask           ( void * pvParameters );
extern void vShadowDeviceTask            ( void * pvParameters );
extern void vOTAUpdateTask               ( void * pvParameters );
extern void vEnvironmentSensorPublishTask( void * pvParameters );
extern void vMotionSensorsPublish        ( void * pvParameters );
extern void vSNTPTask                    ( void * pvParameters );
extern void prvFleetProvisioningTask     ( void * pvParameters );
extern void vDefenderAgentTask           ( void * pvParameters );
extern void vLEDTask                     ( void * pvParameters );
extern void vButtonTask                  ( void * pvParameters );
extern void vHAConfigPublishTask         ( void * pvParameters );
/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
  LogError("Malloc Fail\n");
  vDoSystemReset();
}
/* USER CODE END 5 */

/* USER CODE BEGIN 2 */
void vApplicationIdleHook(void)
{
  vPetWatchdog();
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  taskENTER_CRITICAL();

  LogSys("Stack overflow in %s", pcTaskName);
  (void) xTask;

  vDoSystemReset();

  taskEXIT_CRITICAL();
}
/* USER CODE END 4 */

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
  return 0;
}

uint32_t checkAndClearResetFlags(void)
{
  uint32_t reset_flags = HAL_RCC_GetResetSource();

  if(reset_flags & RCC_RESET_FLAG_PIN)
  {
    LogInfo("Reset source: PIN");
  }

  if(reset_flags & RCC_RESET_FLAG_PWR)
  {
    LogInfo("Reset source: BOR or POR/PDR");
  }



  if(reset_flags & RCC_RESET_FLAG_SW)
  {
    LogInfo("Reset source: Software");
  }

  if(reset_flags & RCC_RESET_FLAG_IWDG)
  {
    LogInfo("Reset source: IWDG");
  }

  if(reset_flags & RCC_RESET_FLAG_WWDG)
  {
    LogInfo("Reset source: WWDG");
  }

  if(reset_flags & RCC_RESET_FLAG_LPWR)
  {
    LogInfo("Reset source: Low power");
  }

  /* Clear all reset flags */
  __HAL_RCC_CLEAR_RESET_FLAGS();

}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  /* Initialize uart for logging before cli is up and running */
  vInitLoggingEarly();

  vLoggingInit();

  LogInfo("HW Init Complete.");

  checkAndClearResetFlags();

  xTaskCreate(StartDefaultTask, "DefaultTask", 2 * configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

  vTaskStartScheduler();
}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief Function implementing the defaultTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  char * pucMqttEndpoint = NULL;
  size_t uxMqttEndpointLen = -1;
#if defined(DEMO_FLEET_PROVISION) && !defined(__USE_STSAFE__)
  BaseType_t xSuccess = pdTRUE;
  uint32_t provisioned = 0;
#endif

#if defined(__USE_STSAFE__)
  bool stsafe_status;

  stsafe_status = SAFEA1_Init();

  if (stsafe_status)
  {
    LogInfo("STSAFE-A1xx initialized successfully");
  }
  else
  {
    LogError("STSAFE-A1xx NOT initialized");
  }
#endif

#if defined(LFS_CONFIG)
  int xMountStatus;
#endif
  (void) argument;

  LogInfo("Task started: %s\n", __func__);

  xSystemEvents = xEventGroupCreate();

  xTaskCreate(Task_CLI, "cli", TASK_STACK_SIZE_CLI, NULL, TASK_PRIO_CLI, NULL);

#if defined(LFS_CONFIG)
  xMountStatus = fs_init();

  if (xMountStatus == LFS_ERR_OK)
  {
    LogInfo("File System mounted.");
    (void) xEventGroupSetBits(xSystemEvents, EVT_MASK_FS_READY);

    KVStore_init();

    pucMqttEndpoint = KVStore_getStringHeap( CS_CORE_MQTT_ENDPOINT, &uxMqttEndpointLen );

#if DEMO_OTA
    if ((uxMqttEndpointLen>0) && (uxMqttEndpointLen < 0xffffffff))
    {
      /* If we are connecting to AWS */
      if (strstr(pucMqttEndpoint, "amazonaws") != NULL)
      {
        otaPal_EarlyInit();
      }
    }
#endif
  }
  else
  {
    LogError("Failed to mount file system.");
  }
#endif
  (void) xEventGroupClearBits( xSystemEvents, EVT_MASK_NET_CONNECTED );

#if !defined(__USE_STSAFE__)
  size_t xLength;

  KVStore_getStringHeap(CS_CORE_THING_NAME, &xLength);

  if ((xLength == 0) || (xLength == -1))
  {
    char *democonfigFP_DEMO_ID = pvPortMalloc(democonfigMAX_THING_NAME_LENGTH);

#if defined(HAL_ICACHE_MODULE_ENABLED)
  HAL_ICACHE_Disable();
#endif

    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

#if defined(HAL_ICACHE_MODULE_ENABLED)
  HAL_ICACHE_Enable();
#endif

    snprintf(democonfigFP_DEMO_ID, democonfigMAX_THING_NAME_LENGTH, democonfigDEVICE_PREFIX"-%08X%08X%08X", (int)uid0, (int)uid1, (int)uid2);

    /* Update the KV Store */
    KVStore_setString(CS_CORE_THING_NAME, democonfigFP_DEMO_ID);


#if defined(DEMO_FLEET_PROVISION) && !defined(__USE_STSAFE__)
    KVStore_setUInt32(CS_PROVISIONED, 0);
    KVStore_setString(CS_CORE_THING_NAME, democonfigFP_DEMO_ID);
#endif

    /* Update the KV Store */
    KVStore_xCommitChanges();

    vPortFree(democonfigFP_DEMO_ID);
  }
#endif

#if defined(MXCHIP)
  xTaskCreate(net_main, "MxNet", TASK_STACK_SIZE_MXCHIP, NULL, TASK_PRIO_MXCHIP, NULL);
#endif

#if defined(ST67W6X_NCP)
  xTaskCreate(W6X_WiFi_Task, "W6xNet", TASK_STACK_SIZE_W6X, NULL, TASK_PRIO_W6X, NULL);
#endif

#if defined(ETHERNET)
  xTaskCreate( &net_main, "EthNet", TASK_STACK_SIZE_NET_ETH, NULL, TASK_PRIO_NET_ETH, NULL );
#endif

#if DEMO_ECHO_SERVER
  xTaskCreate(vEchoServerTask, "EchoServer", TASK_STACK_SIZE_MQTT_AGENT, NULL, TASK_PRIO_MQTTA_AGENT, NULL);
#endif

#if DEMO_ECHO_CLIENT
  xTaskCreate(vEchoClientTask, "EchoClient", TASK_STACK_SIZE_MQTT_AGENT, NULL, TASK_PRIO_MQTTA_AGENT, NULL);
#endif

#if (DEMO_PING && !defined(ST67W6X_NCP))
  xTaskCreate(ping_task, "PingTask", TASK_STACK_SIZE_PING, NULL, TASK_PRIO_PING, NULL);
#endif

#if MQTT_ENABLED
  xTaskCreate(vMQTTAgentTask, "MQTTAgent", TASK_STACK_SIZE_MQTT_AGENT, NULL, TASK_PRIO_MQTTA_AGENT, NULL);
#endif

#if defined(DEMO_FLEET_PROVISION) && !defined(__USE_STSAFE__)
  provisioned = KVStore_getUInt32( CS_PROVISIONED, &( xSuccess ) );

  if(provisioned == 0)
  {
    xTaskCreate(prvFleetProvisioningTask, "FleetProv", TASK_STACK_SIZE_fleetProvisioning, NULL, TASK_PRIO_fleetProvisioning, NULL);

    vTaskDelete( NULL);
  }
#endif



#if DEMO_PUB_SUB
  xTaskCreate(vSubscribePublishTestTask, "PubSub", TASK_STACK_SIZE_PUBLISH, NULL, TASK_PRIO_PUBLISH, NULL);
#endif

#if DEMO_ENV_SENSOR
  xTaskCreate(vEnvironmentSensorPublishTask, "EnvSense", TASK_STACK_SIZE_ENV, NULL, TASK_PRIO_ENV, NULL);
#endif

#if DEMO_MOTION_SENSOR
  xTaskCreate(vMotionSensorsPublish, "MotionS", TASK_STACK_SIZE_MOTION, NULL, TASK_PRIO_MOTION, NULL);
#endif

#if DEMO_HOME_ASSISTANT
      xTaskCreate(vHAConfigPublishTask, "HS", TASK_STACK_SIZE_HS, NULL, TASK_PRIO_HS, NULL);
#endif

#if DEMO_LED
      xTaskCreate(vLEDTask, "LEDTask", TASK_STACK_SIZE_LED, NULL, TASK_PRIO_LED, NULL);
#endif

#if DEMO_BUTTON
      xTaskCreate(vButtonTask, "ButtonTask", TASK_STACK_SIZE_BUTTON, NULL, TASK_PRIO_BUTTON, NULL);
#endif

  if ((uxMqttEndpointLen>0) && (uxMqttEndpointLen < 0xffffffff))
  {
    /* If we are connecting to AWS */
    if (strstr(pucMqttEndpoint, "amazonaws") != NULL)
    {
#if DEMO_OTA
      xTaskCreate(vOTAUpdateTask, "OTAUpdate", TASK_STACK_SIZE_OTA, NULL, TASK_PRIO_OTA, NULL);
#endif

#if DEMO_SHADOW
      xTaskCreate(vShadowDeviceTask, "ShadowDevice", TASK_STACK_SIZE_SHADOW, NULL, TASK_PRIO_SHADOW, NULL);
#endif

#if DEMO_DEFENDER && !defined(ST67W6X_NCP)
      xTaskCreate(vDefenderAgentTask, "AWSDefender", TASK_STACK_SIZE_DEFENDER, NULL, TASK_PRIO_DEFENDER, NULL);
#endif
    }
  }

  if(pucMqttEndpoint != NULL)
  {
    vPortFree(pucMqttEndpoint);
  }

#if DEMO_SNTP
  xTaskCreate(vSNTPTask, "vSNTPTask", TASK_STACK_SIZE_SNTP, NULL, TASK_PRIO_SNTP, NULL);
#endif

  /* Infinite loop */
  for (;;)
  {
    vTaskDelete( NULL);
  }
  /* USER CODE END defaultTask */
}


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
#if defined(LFS_CONFIG)
lfs_t* pxGetDefaultFsCtx(void)
{
  while (pxLfsCtx == NULL)
  {
    LogDebug( "Waiting for FS Initialization." );
    /* Wait for FS to be initialized */
    vTaskDelay(1000);
    /*TODO block on an event group bit instead */
  }

  return pxLfsCtx;
}

static int fs_init(void)
{
  static lfs_t xLfsCtx = { 0 };
  struct lfs_info xDirInfo = { 0 };

  /* Block time of up to 1 s for filesystem to initialize */
#if defined(LFS_USE_INTERNAL_NOR)
  const struct lfs_config *pxCfg = pxInitializeInternalFlashFs (pdMS_TO_TICKS(30 * 1000));
#elif defined(HAL_OSPI_MODULE_ENABLED)
  const struct lfs_config *pxCfg = pxInitializeOSPIFlashFs     (pdMS_TO_TICKS(30 * 1000));
#elif defined(HAL_XSPI_MODULE_ENABLED)
  const struct lfs_config *pxCfg = pxInitializeXSPIFlashFs     (pdMS_TO_TICKS(30 * 1000));
#endif

  /* mount the filesystem */
  int err = lfs_mount(&xLfsCtx, pxCfg);

  /* format if we can't mount the filesystem
   * this should only happen on the first boot
   */
  if (err != LFS_ERR_OK)
  {
    LogError("Failed to mount partition. Formatting...");
    err = lfs_format(&xLfsCtx, pxCfg);

    if (err == 0)
    {
      err = lfs_mount(&xLfsCtx, pxCfg);
    }

    if (err != LFS_ERR_OK)
    {
      LogError("Failed to format littlefs device.");
    }
  }

  if (lfs_stat(&xLfsCtx, "/cfg", &xDirInfo) == LFS_ERR_NOENT)
  {
    err = lfs_mkdir(&xLfsCtx, "/cfg");

    if (err != LFS_ERR_OK)
    {
      LogError("Failed to create /cfg directory.");
    }
  }

  if (lfs_stat(&xLfsCtx, "/ota", &xDirInfo) == LFS_ERR_NOENT)
  {
    err = lfs_mkdir(&xLfsCtx, "/ota");

    if (err != LFS_ERR_OK)
    {
      LogError("Failed to create /ota directory.");
    }
  }

  if (err == 0)
  {
    /* Export the FS context */
    pxLfsCtx = &xLfsCtx;
  }

  return err;
}
#endif
static uint32_t DisableSuppressTicksAndSleepBm;

void DisableSuppressTicksAndSleep(uint32_t bitmask)
{
  taskENTER_CRITICAL();

  DisableSuppressTicksAndSleepBm |= bitmask;

  taskEXIT_CRITICAL();
}

void EnableSuppressTicksAndSleep(uint32_t bitmask)
{
  taskENTER_CRITICAL();

  DisableSuppressTicksAndSleepBm  &= (~bitmask);

  taskEXIT_CRITICAL();
}
/* USER CODE END Application */

