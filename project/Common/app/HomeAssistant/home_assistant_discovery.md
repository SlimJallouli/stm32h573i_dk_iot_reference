# MQTT Auto-Discovery with Home Assistant

This document describes the MQTT topic structure and Home Assistant discovery configuration for your STM32-based IoT device. It also includes a Python script to generate and publish all discovery messages.

---

## Device Identity

All STM32 device IDs follow the pattern: `stm32x123-< serial_number >`

Example:

- Without STSAFE : `stm32h573-002C005B3332511738363236`
- With STSAFEA-110: `eval3-0209203A825AD42AC20139`
- With STSAFEA-120: `eval5-0209203A825AD42AC20139`
- With STSAFEA-TPM: `ST1-TPM-TCA01-EBD60101EE7B88`
---

## MQTT Topic Overview

### 1. LED Control

- **Command topic**: `stm32h573-< serial >/led/desired`  
- **State topic**: `stm32h573-< serial >/led/reported`

#### Example Published Payload:
```json
{
  "ledStatus": {
    "reported": "OFF"
  }
}
```
Or
```json
{
  "ledStatus": {
    "reported": "ON"
  }
}
```
Valid Command Payloads:

```
OFF 
```

Or

```
ON
```

2. Environmental Sensor Payload
Topic: stm32h573-< serial >/sensor/env

```json
{
  "temp_0_c": 23.5,
  "rh_pct": 42.7,
  "temp_1_c": 24.1,
  "baro_mbar": 1012.8
}
```

3. Motion Sensor Payload
Topic: stm32h573-< serial >/sensor/motion

```json
{
  "acceleration_mG": { "x": 123, "y": -98, "z": 1010 },
  "gyro_mDPS": { "x": 25, "y": -30, "z": 12 },
  "magnetometer_mGauss": { "x": 48, "y": 27, "z": -12 }
}
```

4. Push Button (Planned)
Topic: stm32h573-< serial >/button_status

```json
{
  "buttonStatus": {
    "reported": "OFF"
  }
}
```

## Home Assistant MQTT Discovery Topics
Home Assistant uses MQTT discovery to automatically register devices and sensors. Each discovery message must be published to:

```
homeassistant/< component >/< device_id >_< sensor >/config
```

## Button

Example of homeassistant config Topic: 

```
homeassistant/binary_sensor/stm32h573-002C005B3332511738363236_button/config
```
publish Topic: 
Use retain = true when publishing.

With payload:

```json
{
  "name": "Button",
  "unique_id": "stm32h573-002C005B3332511738363236_button",
  "state_topic": "stm32h573-002C005B3332511738363236/sensor/button/reported",
  "value_template": "{{ value_json.buttonStatus.reported }}",
  "payload_on": "ON",
  "payload_off": "OFF",
  "device_class": "occupancy",
  "retain": true
}
```

## LED 

Example of homeassistant config Topic: 

```
homeassistant/switch/stm32h573-002C005B3332511738363236_led/config
```

publish Topic: 
Use retain = true when publishing.

With payload:

```json
{
  "name": "LED",
  "unique_id": "stm32h573-002C005B3332511738363236_led",
  "command_topic": "stm32h573-002C005B3332511738363236/led/desired",
  "state_topic": "stm32h573-002C005B3332511738363236/led/reported",
  "value_template": "{{ value_json.ledStatus.reported }}",
  "payload_on": "ON",
  "payload_off": "OFF",
  "state_on": "ON",
  "state_off": "OFF",
  "retain": true
}
```

## Environmental Sensors (Temp/Humidity/Pressure)

homeassistant config Topic: 
```
Example of homeassistant/sensor/stm32h573-002C005B3332511738363236_<sensor>/config 

```

publish Topic: 
Use retain = true when publishing.

```
stm32h573-002C005B3332511738363236/env_sensor_data
```

|Sensor	     | Name	       | Value     | Template                   |	Unit	    | Device Class |
|:---------  |:---------   |:--------- |:---------                  |:--------- |:---------    |       
| temp0	     | Temperature | Sensor 0  |	{{ value_json.temp_0_c }} |	°C        |	temperature  |
| temp1	     | Temperature | Sensor 1	 | {{ value_json.temp_1_c }}	| °C        | temperature  |
| humidity	 | Humidity	   | Sensor 2  | {{ value_json.rh_pct }}	  | %	        | humidity     |
| pressure	 | Barometric  | Sensor 3  | 	{{ value_json.baro_mbar }}| mbar      | Pressure     |

```json
[
  {
    "name": "Temperature Sensor 0",
    "unique_id": "stm32h573-002C005B3332511738363236_temp0",
    "state_topic": "stm32h573-002C005B3332511738363236/env_sensor_data",
    "value_template": "{{ value_json.temp_0_c }}",
    "unit_of_measurement": "°C",
    "device_class": "temperature",
    "retain": true
  },
  {
    "name": "Humidity",
    "unique_id": "stm32h573-002C005B3332511738363236_humidity",
    "state_topic": "stm32h573-002C005B3332511738363236/env_sensor_data",
    "value_template": "{{ value_json.rh_pct }}",
    "unit_of_measurement": "%",
    "device_class": "humidity",
    "retain": true
  },
  {
    "name": "Temperature Sensor 1",
    "unique_id": "stm32h573-002C005B3332511738363236_temp1",
    "state_topic": "stm32h573-002C005B3332511738363236/env_sensor_data",
    "value_template": "{{ value_json.temp_1_c }}",
    "unit_of_measurement": "°C",
    "device_class": "temperature",
    "retain": true
  },
  {
    "name": "Barometric Pressure",
    "unique_id": "stm32h573-002C005B3332511738363236_pressure",
    "state_topic": "stm32h573-002C005B3332511738363236/env_sensor_data",
    "value_template": "{{ value_json.baro_mbar }}",
    "unit_of_measurement": "mbar",
    "device_class": "pressure",
    "retain": true
  }
]
```

## Motion Sensors (Accel, Gyro, Mag)
This one’s fun! Each axis can be registered as a separate sensor in Home Assistant:

homeassistant config Topic: 
```
homeassistant/sensor/stm32h573-002C005B3332511738363236_accel_x/config

```
 
publish Topic: 
Use retain = true when publishing.

 ```
 stm32h573-002C005B3332511738363236/motion_sensor_data
```

```json
[
  {
    "name": "Acceleration X",
    "unique_id": "stm32h573-002C005B3332511738363236_acceleration_mG_x",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.acceleration_mG.x }}",
    "unit_of_measurement": "mG",
    "retain": true
  },
  {
    "name": "Acceleration Y",
    "unique_id": "stm32h573-002C005B3332511738363236_acceleration_mG_y",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.acceleration_mG.y }}",
    "unit_of_measurement": "mG",
    "retain": true
  },
  {
    "name": "Acceleration Z",
    "unique_id": "stm32h573-002C005B3332511738363236_acceleration_mG_z",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.acceleration_mG.z }}",
    "unit_of_measurement": "mG",
    "retain": true
  },
  {
    "name": "Gyroscope X",
    "unique_id": "stm32h573-002C005B3332511738363236_gyro_mDPS_x",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.gyro_mDPS.x }}",
    "unit_of_measurement": "mDPS",
    "retain": true
  },
  {
    "name": "Gyroscope Y",
    "unique_id": "stm32h573-002C005B3332511738363236_gyro_mDPS_y",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.gyro_mDPS.y }}",
    "unit_of_measurement": "mDPS",
    "retain": true
  },
  {
    "name": "Gyroscope Z",
    "unique_id": "stm32h573-002C005B3332511738363236_gyro_mDPS_z",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.gyro_mDPS.z }}",
    "unit_of_measurement": "mDPS",
    "retain": true
  },
  {
    "name": "Magnetometer X",
    "unique_id": "stm32h573-002C005B3332511738363236_magnetometer_mGauss_x",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.magnetometer_mGauss.x }}",
    "unit_of_measurement": "mG",
    "retain": true
  },
  {
    "name": "Magnetometer Y",
    "unique_id": "stm32h573-002C005B3332511738363236_magnetometer_mGauss_y",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.magnetometer_mGauss.y }}",
    "unit_of_measurement": "mG",
    "retain": true
  },
  {
    "name": "Magnetometer Z",
    "unique_id": "stm32h573-002C005B3332511738363236_magnetometer_mGauss_z",
    "state_topic": "stm32h573-002C005B3332511738363236/motion_sensor_data",
    "value_template": "{{ value_json.magnetometer_mGauss.z }}",
    "unit_of_measurement": "mG",
    "retain": true
  }
]
```
