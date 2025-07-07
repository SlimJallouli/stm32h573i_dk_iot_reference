# Button Status Example (`button_task.c`)

This example demonstrates how to monitor the USER_Button status of the STM32 board via MQTT messages. The application reports the button state (ON/OFF) whenever the USER_Button is pressed or released by publishing to a specific MQTT topic.

---

## How It Works

Whenever the USER_Button is pressed or released, the board publishes the button status to the topic:  
`< ThingName >/sensor/button/reported`  
in JSON format.

- The `ThingName` depends on your project configuration:
- Without STSAFE: `stm32h573-002C005B3332511738363236`
- With STSAFEA-110: `eval3-0102203B825BD42BC20554`
- With STSAFEA-120: `eval5-0209203D823AD52A920A39`
- With STSAFEA-TPM: `ST1-TPM-TCA01-ABC60101DD7B33`

---

## MQTT Topic Overview

- **Button state topic:**  
  `< ThingName >/sensor/button/reported`

#### Example state report sent by the board

```json
{
  "buttonStatus": {
    "reported": "OFF"
  }
}
```
or
```json
{
  "buttonStatus": {
    "reported": "ON"
  }
}
```

---

## Monitoring MQTT Messages

You can use any MQTT client to monitor the button status. Below are two recommended web clients:

<details>
  <summary>Option 1: mqtt.cool for test.mosquitto.org</summary>

1. Open [mqtt.cool](https://testclient-cloud.mqtt.cool/)
2. Connect to `test.mosquitto.org` on port `1883`.
3. Subscribe to the topic:  
   `< ThingName >/sensor/button/reported`  
   (e.g. `stm32h573-002C005B3332511738363236/sensor/button/reported`)
4. Press the USER_Button on your board.
5. You will see messages published by your board.

![alt text](../../../assets/mqtt_cool_button_reported.png)

</details>

---

<details>
  <summary>Option 2: MQTTX Web Client for broker.emqx.io</summary>

1. Connect to [mqttx.app web-client](https://mqttx.app/web-client) and connect to `broker.emqx.io` on port `1883`.

![alt text](../../../assets/emqx_mqtt_connect.png)

2. Subscribe to the topic:  
   `< ThingName >/sensor/button/reported`  
   (e.g. `stm32h573-002C005B3332511738363236/sensor/button/reported`)

3. Press the USER_Button on your board.

4. You will see messages published by your board.

![alt text](../../../assets/emqx_mqtt_button_reported.png)

</details>

---

## Firmware overview

The button status firmware listens for hardware events from the USER_Button and reports its state via MQTT. When the button is pressed or released, the firmware publishes the new status to the `<ThingName>/sensor/button/reported` topic in JSON format. This allows remote clients to monitor the button state in real time.

Key features:
- Detects USER_Button press and release events using GPIO interrupts.
- Publishes the current button status (ON/OFF) as a JSON message after every event.
- Uses FreeRTOS tasks, event groups, and the MQTT agent for robust operation.
- Dynamically constructs MQTT topics based on the device's Thing Name.

## Key Functions and Logic

### 1. `void vButtonTask(void *pvParameters)`

- **Main FreeRTOS task function** for button monitoring and MQTT publishing.
- Waits for the MQTT agent to be ready and connected.
- Retrieves the device Thing Name from the key-value store.
- Allocates buffers and creates an event group for button events.
- Registers GPIO interrupt callbacks for button press (falling edge) and release (rising edge).
- Waits for button events using `xEventGroupWaitBits`.
- Publishes the button status (`ON` for released, `OFF` for pressed) as a JSON message to the `<ThingName>/sensor/button/reported` topic.
- Logs the result of each publish operation.

---

### 2. `static void user_button_rising_event(void *pvContext)`

- **GPIO interrupt callback** for the rising edge (button released).
- Sets the `BUTTON_RISING_EVENT` bit in the event group from ISR context.

---

### 3. `static void user_button_falling_event(void *pvContext)`

- **GPIO interrupt callback** for the falling edge (button pressed).
- Sets the `BUTTON_FALLING_EVENT` bit in the event group from ISR context.

---

### 4. `static MQTTStatus_t prvPublishToTopic(MQTTQoS_t xQoS, char *pcTopic, uint8_t *pucPayload, size_t xPayloadLength)`

- Publishes a payload to the specified MQTT topic with the given QoS.
- Waits for the publish operation to complete and checks the result.
- Retries if the MQTT agent's command queue is full.

---

### 5. `static void prvPublishCommandCallback(MQTTAgentCommandContext_t *pxCommandContext, MQTTAgentReturnInfo_t *pxReturnInfo)`

- Callback executed when the MQTT broker acknowledges the publish message.
- Notifies the task that initiated the publish operation of the result.

---

**Summary:**  
The button task enables real-time reporting of the USER_Button state via MQTT. It uses FreeRTOS tasks, event groups, and GPIO interrupts for efficient operation, and integrates with the MQTT agent
