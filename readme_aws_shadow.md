# AWS IoT Device Shadow Example

This document explains what AWS IoT Device Shadow is and how it is used in the STM32 FreeRTOS IoT Reference project (`shadow_device_task.c`).

---

## What is AWS IoT Device Shadow?

[AWS IoT Device Shadow](https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html) is a managed AWS IoT service that provides a persistent, virtual representation (shadow) of each device connected to AWS IoT. The shadow stores the device's last reported state and desired future state as JSON documents, allowing cloud applications and devices to synchronize and interact even when the device is offline.

---

## How is Device Shadow Used in This Project?

The STM32 FreeRTOS IoT Reference project includes a Device Shadow example (`shadow_device_task.c`) that demonstrates:

- How to assemble and subscribe to AWS IoT Device Shadow MQTT topics.
- How to report the device's state (e.g., power status) to the shadow.
- How to receive and process desired state changes from the cloud.
- How to synchronize the device's state with the shadow document.

---

## Example: Power State Synchronization

The example manages a `powerOn` state (e.g., for an onboard LED):

- The device reports its current `powerOn` state to the shadow using the topic:  
  ```
  $aws/things/<thing-name>/shadow/update
  ```
- The device subscribes to shadow topics for delta, accepted, and rejected updates:
  - `$aws/things/<thing-name>/shadow/update/delta`
  - `$aws/things/<thing-name>/shadow/update/accepted`
  - `$aws/things/<thing-name>/shadow/update/rejected`
- When a desired state change is published to the shadow (from the AWS Console or another client), the device receives a delta update, applies the change (e.g., turns the LED on/off), and reports the new state.

---

## Example Shadow Document

### Reported State (sent by the device):

```json
{
  "state": {
    "reported": {
      "powerOn": "1",
      "Board": "STM32H573I-DK",
      "Connectivity": "Ethernet"
    }
  },
  "clientToken": "021909"
}
```

### Desired State (set from the cloud):

Send a MQTT message to`$aws/things/<thing-name>/shadow/update`

```json
{
  "state": {
    "desired": {
      "powerOn": "1"
    }
  }
}
```

Or 

```json
{
  "state": {
    "desired": {
      "powerOn": "1"
    }
  }
}
```

The LED will turn On/Off depending on the JSON message you sent. The board will send bak a message confirming the `reported` state.

The `"Board": "STM32H573I-DK"` and `"Connectivity": "ST67_NCP"` fileds depends on the project configuration you have selected

```json
{
  "state": {
    "reported": {
      "powerOn": "1",
      "Board": "STM32H573I-DK",
      "Connectivity": "ST67_NCP"
    }
  },
  "clientToken": "021909"
}
```
---

## How to Use

1. **Provision your device** with AWS IoT Core and ensure it is connected.
2. **Open the AWS IoT Console** and navigate to your Thing.
3. Go to the **Device Shadow** section.
4. You can view the current reported state and set a desired state.
5. When you update the desired state, the device will receive a delta update, apply the change, and update the reported state.

---

## Why Use Device Shadow?

- **Synchronize device state** between the cloud and device, even when the device is offline.
- **Remotely control devices** by setting desired states from the cloud.
- **Monitor device status** and receive real-time updates.

---
