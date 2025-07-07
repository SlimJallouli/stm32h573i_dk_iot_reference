# AWS IoT Device Defender Example

This document explains what AWS IoT Device Defender is and how it can be used with your STM32 FreeRTOS IoT Reference project.

---

## What is AWS IoT Device Defender?

[AWS IoT Device Defender](https://docs.aws.amazon.com/iot/latest/developerguide/device-defender.html) is a fully managed AWS IoT service that helps you secure your fleet of IoT devices. It continuously audits your IoT configurations to ensure they aren't deviating from security best practices, and it detects abnormal device behavior to help you identify potential security issues.

Device Defender provides two main capabilities:

- **Audit:**  
  Continuously checks your IoT resources and policies for security vulnerabilities or misconfigurations (such as overly permissive policies, disabled logging, etc.).

- **Detect:**  
  Monitors device behaviors (such as traffic patterns, message rates, and connection attempts) and alerts you when anomalies are detected that could indicate a compromised device.

---

## How is Device Defender Used in This Project?

The STM32 FreeRTOS IoT Reference project includes support for AWS IoT Device Defender. When enabled and provisioned, your device can:

- Report security metrics (such as open ports, listening IP addresses, and network statistics) to AWS IoT Device Defender using MQTT.
- Allow you to monitor your device's security posture and detect abnormal behavior from the AWS IoT Console.

---

## How to Enable and Use Device Defender

1. **Provision your device** with AWS IoT Core as described in the main [README](../../readme.md).
2. **Ensure the Device Defender CMSIS pack is installed:**  
   - [AWS IoT Device Defender 4.1.1](https://d1pm0k3vkcievw.cloudfront.net/AWS.AWS_IoT_Device_Defender.4.1.1.pack)
3. **Build and flash the firmware** with Device Defender support enabled (see your project configuration).
4. **Monitor metrics in AWS Console:**  
   - Go to the AWS IoT Console.
   - Navigate to **Defend** > **Detect** to view reported metrics and alerts for your device.

---

## Example Device Defender Report

The device will periodically publish JSON reports to the following MQTT topic:

```
$aws/things/<thing-name>/defender/metrics/json
```

Example payload:

```json
{
  "header": {
    "report_id": 123456789,
    "version": "1.0"
  },
  "metrics": {
    "listeningTcpPorts": {
      "ports": [8883]
    },
    "listeningUdpPorts": {
      "ports": []
    },
    "networkStats": {
      "bytesIn": 10240,
      "bytesOut": 20480,
      "packetsIn": 100,
      "packetsOut": 200
    }
  }
}
```

---

## Why Use Device Defender?

- **Continuous security auditing** of your IoT fleet
- **Early detection** of abnormal device behavior
- **Automated alerts** for potential security issues
- **Helps meet compliance requirements** for IoT deployments

---

For more information, see