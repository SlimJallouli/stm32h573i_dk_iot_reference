import paho.mqtt.publish as publish
import json

# === CONFIG ===
broker = "your-mqtt-broker.local"  # e.g., "homeassistant.local" or IP address
port = 1883
device_id = "stm32h573-002C005B3332511738363236"
retain = True

# === Utility ===
def pub(topic_suffix, payload):
    topic = f"homeassistant/{topic_suffix}"
    publish.single(topic, json.dumps(payload), hostname=broker, port=port, retain=retain)
    print(f"Published to {topic}")

# === LED Switch ===
pub(f"switch/{device_id}_led/config", {
    "name": "STM32H573 LED",
    "unique_id": f"{device_id}_led",
    "command_topic": f"{device_id}/led_set",
    "state_topic": f"{device_id}/led_status",
    "value_template": "{{ value_json.ledStatus.reported }}",
    "payload_on": "ON",
    "payload_off": "OFF",
    "state_on": "ON",
    "state_off": "OFF",
    "retain": True
})

# === Environment Sensors ===
env_sensor_topic = f"{device_id}/env_sensor_data"
sensors = [
    ("temp0", "Temperature Sensor 0", "temp_0_c", "°C", "temperature"),
    ("temp1", "Temperature Sensor 1", "temp_1_c", "°C", "temperature"),
    ("humidity", "Humidity", "rh_pct", "%", "humidity"),
    ("pressure", "Barometric Pressure", "baro_mbar", "mbar", "pressure")
]
for key, name, template_field, unit, dev_class in sensors:
    pub(f"sensor/{device_id}_{key}/config", {
        "name": name,
        "unique_id": f"{device_id}_{key}",
        "state_topic": env_sensor_topic,
        "value_template": f"{{{{ value_json.{template_field} }}}}",
        "unit_of_measurement": unit,
        "device_class": dev_class,
        "retain": True
    })

# === Motion Sensors ===
motion_topic = f"{device_id}/motion_sensor_data"
motion_axes = {
    "acceleration_mG": "Acceleration",
    "gyro_mDPS": "Gyroscope",
    "magnetometer_mGauss": "Magnetometer"
}
for sensor, label in motion_axes.items():
    for axis in ("x", "y", "z"):
        key = f"{sensor}_{axis}"
        pub(f"sensor/{device_id}_{key}/config", {
            "name": f"{label} {axis.upper()}",
            "unique_id": f"{device_id}_{key}",
            "state_topic": motion_topic,
            "value_template": f"{{{{ value_json.{sensor}.{axis} }}}}",
            "unit_of_measurement": "mG" if "acceleration" in sensor else "mDPS" if "gyro" in sensor else "mG",
            "retain": True
        })

# === Button Sensor (Binary) ===
pub(f"binary_sensor/{device_id}_button/config", {
    "name": "Push Button",
    "unique_id": f"{device_id}_button",
    "state_topic": f"{device_id}/button_status",
    "value_template": "{{ value_json.buttonStatus.reported }}",
    "payload_on": "ON",
    "payload_off": "OFF",
    "device_class": "occupancy",
    "retain": True
})
