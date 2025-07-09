# Provision with AWS using STSAFE

This provisioning method is supported by the following project configurations:

|       Build Config          | Provisioning method       |
|:---------                   |:----------                |
| Ethernet_STSAFEA110         | MAR, JITP, JITR           |
| Ethernet_STSAFEA120         | MAR, JITP, JITR           |
| MXCHIP_STSAFEA110           | MAR, JITP, JITR           |
| MXCHIP_STSAFEA120           | MAR, JITP, JITR           |

## 1. Hardware Setup

If you’ve selected MXCHIP, Connect the Wi-Fi module to either the STMod+ connector on the board.

Depending on the project configuration, connect the  [X-NUCLEO-SAFEA1](https://www.st.com/en/ecosystems/x-nucleo-safea1.html) or [X-NUCLEO-ESE01A1](https://www.st.com/en/ecosystems/x-nucleo-ese01a1.html) to your board.

If you’re using the Ethernet configuration, connect the Ethernet cable to the board’s Ethernet port.

Then, in all cases, connect the board to your PC via the ST-Link USB port to power it and enable programming/debugging.

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

## 3. Run and Test the Examples

After provisioning your board, you can run and test the application features. Refer to the [Run and Test the Examples](readme.md#7-run-and-test-the-examples) section in the main README for details.

---

[⬅️ Back to Main README - Run and Test the Examples](readme.md#7-run-and-test-the-examples)