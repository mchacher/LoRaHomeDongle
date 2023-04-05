# LoRaHomeDongle

## Contents
- [Whats is LoRaHomeDongle?](#what-is-lorahomedongle)
- [Software requirements?](#software-requirements)
- [Hardware requirements?](#hardware-requirements)
- [Design principles?](#design-principles)


<!-- - [Usage and installation](#about-lora)
- [References](#references) -->

## What is LoRaHomeDongle?

LoRaHomeDongle is a USB adapter which enable bridging a lora home network and software applications (home automation). It is acting as the lora home gateway of the network, sending and receiving lora home packet over USB. It is intended to be used with [LoRa2MQTTpy service](https://github.com/mchacher/lora2mqttpy) simplifing software application integration with MQTT.

The dongle is managing the data link layer of the lora home communication protocol. The main features are:
- handle RX packets (check data integrity with a checksum, reply with ACK is any required, forward packet over USB uart)
- handle TX packets (receive packet over the uart, add checksum, and send them over lora)
- report its status with hearbeat system messages sent over uart
- provide a setting interface with system messages (lora communication, lora home network id)

## Hardware requirements

ESP32 based hardware with a semtech SX12xx lora based communication module.
Typically Heltec LoRa 32 or TTGO LoRa32.

## Software requirements

The repository contains a PlatformIO project.
VSCode IDE shall be used with PlatformIO extension.
LoRaHomeDongle is leveraging Arduino framework, and FreeRTOS as underlying framework.

## Design principles



<!-- 
### About LoRa ###

Users, geeks, makers are commonly facing communication distance issue when integrating  short-range based technology into their sensors or actuators (Bluetooth, Zigbee or Z Wave). [LoRa technology](https://en.wikipedia.org/wiki/LoRa) is great to overcome this issue, while still being power efficient.

When it is used by telecom operators to deploy commercial long-range networks, LoRa is based on [LoRaWAN](https://en.wikipedia.org/wiki/LoRa#LoRaWAN) network, but anyone can use LoRa without LoRaWAN and deploy his own private network in a home or a building.

###  Principles ###

The principles are the following:
- LoRa nodes are sending JSON messages over LoRa to the gateway
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



## References ##

LoRa2MQTT is using many libraries available on GitHub for Arduino.
A big thank you to all the authors:
- [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa). Author @sandeepmistry

# LoRaHomeDongle -->
