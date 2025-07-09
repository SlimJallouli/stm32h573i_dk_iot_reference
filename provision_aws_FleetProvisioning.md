# Provision with AWS using Fleet Provisioning

This provisioning method is supported by the following project configurations:

|       Build Config          | Provisioning method       |
|:---------                   |:----------                |
| Ethernet_FleetProvisioning  | Fleet Provisioning        |
| MXCHIP_FleetProvisioning    | Fleet Provisioning        |


## 1. Hardware Setup

If you’ve selected the MXCHIP configuration, connect the Wi-Fi module to the STMod+ connector on the board.

If you’re using the Ethernet configuration, connect the Ethernet cable to the board’s Ethernet port.

Then, in all cases, connect the board to your PC via the ST-Link USB port to power it and enable programming/debugging.

## 2. Fleet Provisioning

[Fleet Provisioning](https://docs.aws.amazon.com/iot/latest/developerguide/provision-wo-cert.html#claim-based) is a feature of AWS IoT Core that automates the end-to-end device onboarding process. It securely delivers unique digital identities to devices, validates device attributes via Lambda functions, and sets up devices with all required permissions and registry metadata. This method is ideal for large-scale device deployments.

Follow this [link](https://github.com/SlimJallouli/stm32mcu_aws_fleetProvisioning) for instructions

## 3. Run and Test the Examples

After provisioning your board, you can run and test the application features. Refer to the [Run and Test the Examples](readme.md#7-run-and-test-the-examples) section in the main README for details.

---

[⬅️ Back to Main README - Run and Test the Examples](readme.md#7-run-and-test-the-examples)