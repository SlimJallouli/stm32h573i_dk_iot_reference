# Provision with AWS
The provisioning method depends on the project configuration you have selected

|       Build Config          | Provisioning method       |
|:---------                   |:----------                |
| Ethernet                    | Single Thing Provisioning |
| Ethernet_FleetProvisioning  | Fleet Provisioning        |
| Ethernet_STSAFEA110         | MAR, JITP, JITR           |
| Ethernet_STSAFEA120         | MAR, JITP, JITR           |
| MXCHIP                      | Single Thing Provisioning |
| MXCHIP_FleetProvisioning    | Fleet Provisioning        |
| MXCHIP_STSAFEA110           | MAR, JITP, JITR           |
| MXCHIP_STSAFEA120           | MAR, JITP, JITR           |
| ST67_NCP                    | Single Thing Provisioning |

## 1. Hardware Setup

If you’ve selected the MXCHIP or ST67_NCP configuration, connect the Wi-Fi module to either the STMod+ or Arduino connector on the board.

If you’re using the Ethernet configuration, connect the Ethernet cable to the board’s Ethernet port.

Then, in all cases, connect the board to your PC via the ST-Link USB port to power it and enable programming/debugging.

## 2. Flash and run the project

## [Single Thing Provisioning](https://docs.aws.amazon.com/iot/latest/developerguide/single-thing-provisioning.html)

[Single Thing Provisioning](https://docs.aws.amazon.com/iot/latest/developerguide/single-thing-provisioning.html), is a method used to provision individual IoT devices in AWS IoT Core. This method is ideal for scenarios where you need to provision devices one at a time.

In this method you have two options. Automated using Python script or manual.

1. [Provision automatically with provision.py](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/Getting_Started_Guide.md#option-8a-provision-automatically-with-provisionpy)

This method involves using a Python script [(provision.py)](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/tools/provision.py) to automate the onboarding process of IoT devices to AWS IoT Core. It simplifies the process by handling the device identity creation, registration, and policy attachment automatically. follow this [link](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/Getting_Started_Guide.md#option-8a-provision-automatically-with-provisionpy) for instructions

2. [Provision Manually via CLI](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/Getting_Started_Guide.md#option-8b-provision-manually-via-cli)

This method requires manually provisioning devices using the AWS Command Line Interface (CLI). It involves creating device identities, registering them with AWS IoT Core, and attaching the necessary policies for device communication. Follow this  [link](https://github.com/FreeRTOS/iot-reference-stm32u5/blob/main/Getting_Started_Guide.md#option-8b-provision-manually-via-cli) for instructions.

**Delete old certs from ST67 internal file system**

If you are using the ST67_NCP configuration, it’s important to ensure that all previously stored certificates especially **corePKCS11_CA_Cert.dat**, **corePKCS11_Cert.dat**, and **corePKCS11_Key.dat** are removed from the module’s internal file system before importing new ones. This step is necessary to allow the firmware to load the updated certificates and private key into the ST67 module, which are then used for establishing the TLS/MQTT connection.

On the serial terminal connected to your board, Type the following command to list all files currently stored in the module:

```
w6x_fs ls
```

![alt text](assets/w6x_fs_ls.png)

Delete any existing file using the following command:
```
w6x_fs rm <filename>
```

![alt text](assets/w6x_fs_rm.png)

**Reset the board**

In the serial terminal connected to your board, type the following command:

```
reset
```

This will reboot the device. Upon startup, the firmware will use the newly imported TLS client certificate and configuration to securely connect to the MQTT broker.

For all standard configurations, the host microcontroller handles the TLS and MQTT stack directly.

For the ST67_NCP configuration, after each boot, the firmware checks for the presence of **corePKCS11_CA_Cert.dat**, **corePKCS11_Cert.dat**, and **corePKCS11_Key.dat** in the ST67's internal file system. If any of these files are missing, the firmware copies the corresponding certificates and private key from the microcontroller's internal file system to ST67.

Once connected, you should see confirmation messages in the terminal indicating a successful TLS handshake and MQTT session establishment.

![alt text](assets/mqtt_connection.png)

## [Fleet Provisioning](https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html#claim-based)
[Fleet Provisioning](https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html#claim-based) is a feature of AWS IoT Core that automates the end-to-end device onboarding process. It securely delivers unique digital identities to devices, validates device attributes via Lambda functions, and sets up devices with all required permissions and registry metadata. This method is ideal for large-scale device deployments.

Follow this [link](https://github.com/SlimJallouli/stm32mcu_aws_fleetProvisioning) for instructions

### [Multi-Account Registration](https://aws.amazon.com/about-aws/whats-new/2020/04/simplify-iot-device-registration-and-easily-move-devices-between-aws-accounts-with-aws-iot-core-multi-account-registration/)
[Multi-Account Registration (MAR)](https://aws.amazon.com/about-aws/whats-new/2020/04/simplify-iot-device-registration-and-easily-move-devices-between-aws-accounts-with-aws-iot-core-multi-account-registration/) registration method uses a secure element [(STSAFE)](https://www.st.com/en/secure-mcus/stsafe-a110.html) for added security. The device certificate, private key, and configuration parameters are saved on [(STSAFE)](https://www.st.com/en/secure-mcus/stsafe-a110.html). This method simplifies device registration and allows for easy movement of devices between multiple AWS accounts. It eliminates the need for a Certificate Authority (CA) to be registered with AWS IoT. The secure element provides additional security by storing sensitive information securely on the device. This method is ideal for large-scale device deployments.

Follow this [link](https://github.com/stm32-hotspot/stm32mcu_aws_mar) for instructions

## [Just-in-Time Provisioning](https://aws.amazon.com/blogs/iot/setting-up-just-in-time-provisioning-with-aws-iot-core/)

[Just-in-Time Provisioning (JITP)](https://aws.amazon.com/blogs/iot/setting-up-just-in-time-provisioning-with-aws-iot-core/) is a method used to automatically provision IoT devices when they first attempt to connect to AWS IoT Core. The [(STSAFE)](https://www.st.com/en/secure-mcus/stsafe-a110.html) module stores the device certificate, private key, and configuration parameters securely, ensuring that the registration process is secure and reliable. This additional layer of security provided by the STSAFE module ensures that sensitive information is kept safe, making it a valuable asset for provisioning IoT devices with AWS IoT Core. This method is ideal for large-scale device deployments.

Follow this [link](https://aws.amazon.com/blogs/iot/setting-up-just-in-time-provisioning-with-aws-iot-core/) for instructions

## [Just-in-Time Registration](https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/)

[Just-in-Time Registration (JITR)](https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/) is a method used by AWS IoT Core to automatically register device certificates when a device first connects to AWS IoT.
The [(STSAFE)](https://www.st.com/en/secure-mcus/stsafe-a110.html) module stores the device certificate, private key, and configuration parameters securely, ensuring that the registration process is secure and reliable. This additional layer of security provided by the STSAFE module ensures that sensitive information is kept safe, making it a valuable asset for provisioning IoT devices with AWS IoT Core. This method is ideal for large-scale device deployments.

Follow this [link](https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/) for instructions


## Run and Test the Examples

After provisioning your board, you can run and test the application features. Refer to the [Run and Test the Examples](readme.md#7-run-and-test-the-examples) section in the main README for details.

---

[⬅️ Back to Main README - Run and Test the Examples](readme.md#7-run-and-test-the-examples)