# LoRaHomeDongle

## Contents
- [Whats is LoRaHomeDongle?](#what-is-lorahomedongle)
- [Software requirements?](#software-requirements)
- [Hardware requirements?](#hardware-requirements)
- [Design principles?](#design-principles)


<!-- - [Usage and installation](#about-lora)
- [References](#references) -->

## What is LoRaHomeDongle?

LoRaHomeDongle is managing LoRa downstream communication of a LoRaHome network.
It is intended to be used with LoRa2MQTTpy service.

The dongle is managing the data link layer of the lora home communication protocol. The main features are:
- handle RX packets (check data integrity with a checksum, reply with ACK is any required, forward packet over USB uart)
- handle TX packets (receive packet over the uart, add checksum, and send them over lora)
- report its status with hearbeat system messages sent over uart
- provide a setting interface with system messages (lora communication, lora home network id)

## Hardware requirements

ESP32 based hardware with a semtech SX12xx lora based communication module.
Typically Heltec LoRa 32 or TTGO LoRa32.

## Software requirements

The repository comes with a full project definition to build it on top of platformio.
VSCode shall be prefered with platformio extension.
LoRaHomeDongle is leveraging Arduino framework, and FreeRTOS as underlying framework.

## Design principles



<!-- 
### About LoRa ###

Users, geeks, makers are commonly facing communication distance issue when integrating  short-range based technology into their sensors or actuators (Bluetooth, Zigbee or Z Wave). [LoRa technology](https://en.wikipedia.org/wiki/LoRa) is great to overcome this issue, while still being power efficient.

When it is used by telecom operators to deploy commercial long-range networks, LoRa is based on [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) network, but anyone can use LoRa without LoRaWAN and deploy his own private network in a home or a building.

###  Principles ###

The principles are the following:
- LoRa nodes are sending JSON messages over LoRa to the LoRa2MQTT gateway
- The JSON messages format is free, except one name-value pair that should be *"node"="name_of_the_node"*.<br/>The name *node* should be the first child, and its value is a unique identifier of the LoRa node. Two nodes shall not have the same *node* value.
```
// example of a thermostat node, reporting
// - 21°c ambiant temperature,
// - an actual setpoint of 22°c,
// - an active state (heating).
{
  "node" = "thermostat",
  "temperature" = "21.2",
  "setpoint" = "22",
  "state" = "true"
}
```

- LoRa2MQTT is forwarding JSON messages to the MQTT broker on *topic lora2mqtt/node_value*
```
// the JSON message is forwarded to lora2mqtt/thermostat
// kindly note that the node field is removed by LoRa2MQTT
{
  "temperature" = "21.2",
  "setpoint" = "22",
  "state" = "true"
}
```
- Conversely, the messages received on _topic lora2mqtt/tonodes_ are forwarded to the LoRa nodes. The same JSON message format applies.
```
// example of a setpoint change request down to 21°c
// the following message is sent to lora2mqtt/tonodes topic, and forwarded on LoRa without modification by LoRa2MQTT
{
  "node" = "thermostat",
  "setpoint" = "21"
}
```

## Usage and installation ##

The project has been tested with ESP32 Heltec LoRa v2 board, but this should work with any « Semtech SX12XX » boards like TTGOv2.

A quite appreciable advantage of these boards is that they are ready to use with the following key features:
- ESP32 micro-controller (with WiFi)
- 8 Mb flash
- Semtech chip SX1276
- external LoRa antenna, IPEX interface
- 0.96 inch OLED display
- Commercial references depending on the sub-giga frequency intended to be used depending on your country

Last but not least you can find these modules or equivalent at a reasonable price on Amazon or Aliexpress.

## References ##

LoRa2MQTT is using many libraries available on GitHub for Arduino.
A big thank you to all the authors:
- [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa). Author @sandeepmistry
- [pubsubclient](https://github.com/knolleary/pubsubclient). Author @knolleary
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson). Author @bblanchon
- [WiFiManager](https://github.com/tzapu/WiFiManager). Author @tzapu
# LoRaHomeDongle -->
