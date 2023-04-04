/**
 * @file lora_home_gateway.cpp
 * @author mchacher
 * @brief Lora Home gateway implmentation
 * Manage RX and TX messages received over Lora data link layer
 * check / add incoming messages
 * manage ack when needed
 * make messages available on fifo queues to applicative layers
 * expose communication API
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <lora_home_gateway.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include "esp_task_wdt.h"
#include "lora_home_configuration.h"
#include "serial_api.h"
#include "dongle_configuration.h"

// White LED management of heltec_wifi_lora_32_V2 board
#define LED_WHITE 25


// rx LoRa packet queue
QueueHandle_t LoRaHomeGateway::rx_packet_queue = xQueueCreate(5, LH_FRAME_MAX_SIZE * sizeof(uint8_t));
// rx LoRa ACK queue
QueueHandle_t LoRaHomeGateway::rx_ack_packet_queue = xQueueCreate(5, LH_FRAME_ACK_SIZE * sizeof(uint8_t));
// tx LoRa queue
QueueHandle_t LoRaHomeGateway::tx_packet_queue = xQueueCreate(5, LH_FRAME_MAX_SIZE * sizeof(uint8_t));

// if running on Core 1 - same as per Arduino Framework
// if running on Core 2 - leverage dual core architecture of ESP32
static int lora_code = 0;
TaskHandle_t task_lora;

// rx_counter - each time a LoRa message is received, counter is incremented
uint32_t LoRaHomeGateway::rx_counter = 0;
// tx_counter - each time a LoRa message is sent out, counter is incremented
uint32_t LoRaHomeGateway::tx_counter = 0;
// err_counter - each time an error is triggered
uint32_t LoRaHomeGateway::err_counter = 0;
// counter used inside Tx LoRaHomeFrame
uint16_t LoRaHomeGateway::packet_id_counter = 0;
// time stamp of the last packet received
unsigned long LoRaHomeGateway::last_packet_ts = millis();
// network id
uint16_t LoRaHomeGateway::network_id = 0;

/**
 * @brief Construct a new LoRaHomeGateway object
 *
 */
LoRaHomeGateway::LoRaHomeGateway()
{
}

/**
 * @brief setup the lora home gateway
 * Start Lora communication in a dedicated task (and core on ESP32)
 * 
 * @param lc lora configuration settings to be used
 * @param network_id the lora home network id to be used
 */
void LoRaHomeGateway::setup(LORA_CONFIGURATION *lc, uint16_t network_id)
{
  // configure Pinout for white LED
  pinMode(LED_WHITE, OUTPUT);
  // set network id
  this->network_id = network_id;
  // setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  while (!LoRa.begin(lc->channel))
  {
    delay(500);
  }

  LoRa.setSpreadingFactor(lc->spreading_factor);
  LoRa.setSignalBandwidth(lc->bandwidth);
  LoRa.setCodingRate4(lc->coding_rate);
  // Change sync word (0xF3) to match the receiver
  // ranges from 0-0xFF
  // LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.enableCrc();

  // set callback handler for LoRa message
  this->enable();

  // start LoRa Task on core 0 (not used by arduino framework)
  xTaskCreatePinnedToCore(
      LoRa.taskRxTx,    /* Task function. */
      "LoRa",           /* name of task. */
      10000,            /* 10kBytes Stack size of task */
      NULL,             /* (void *)this->onReceive, parameter of the task */
      1,                /* priority of the task */
      &task_lora, /* Task handler to keep track of created task */
      lora_code);        /* pin task to core 0, default Arduino setup and main running on core 1 */

  // set in rx mode.
  this->rxMode();
}

/**
 * @brief set the lora home network id
 * 
 * @param network_id network id value
 */
void LoRaHomeGateway::setNetworkID(uint16_t network_id)
{
  this->network_id = network_id;
}

/**
 * @brief put the packet in the Tx Fifo
 * 
 * @param packet lora home packet
 */
void LoRaHomeGateway::putPacket(uint8_t *packet)
{
  bool success = false;
  uint8_t ackBuffer[LH_FRAME_ACK_SIZE];
  uint8_t retry = 0;
  LORA_HOME_PACKET *lora_packet = (LORA_HOME_PACKET *)packet;
  lora_packet->crc16 = crc16_ccitt(packet, sizeof(LORA_HOME_PACKET_HEADER) + lora_packet->header.payloadSize);

  uint8_t raw_packet[LH_FRAME_MAX_SIZE];
  memcpy(raw_packet, packet, sizeof(LORA_HOME_PACKET_HEADER) + lora_packet->header.payloadSize);
  memcpy(&raw_packet[sizeof(LORA_HOME_PACKET_HEADER) + lora_packet->header.payloadSize], &(lora_packet->crc16), 2);
  // send the LoRaHome raw_packet and loop max retry if necessary
  do
  {
    xQueueSend(tx_packet_queue, raw_packet, 0);
    BaseType_t anymsg = xQueueReceive(rx_ack_packet_queue, ackBuffer, pdMS_TO_TICKS(ACK_TIMEOUT));
    if (pdTRUE == anymsg)
    {
      LORA_HOME_PACKET *ack;
      ack = (LORA_HOME_PACKET*) ackBuffer;
      if ((ack->header.nodeIdEmitter == lora_packet->header.nodeIdRecipient) && (ack->header.counter == lora_packet->header.counter))
      {
        success = true;
      }
    }
    retry++;
    // loop if ack not received
  } while ((false == success) && (retry < MAX_RETRY_NO_VALID_ACK));
}

/**
 * @brief pop the LoRaHomeFrame if any available in the Rx message queue
 *
 * @param rxBuffer the lora home packet
 * @return true if a message was available
 * @return false if no message available
 */
bool LoRaHomeGateway::popLoRaHomePayload(uint8_t *rxBuffer)
{
  // uint8_t rxBuffer[LH_FRAME_MAX_SIZE];

  // any LoRa message in the queue?
  // enter critical section to retrieve data in the queue
  BaseType_t anymsg = xQueueReceive(rx_packet_queue, rxBuffer, 0);
  if (pdTRUE == anymsg)
  {
    last_packet_ts = millis();
    return true;
  }
  return false;
}

/**
 * @brief put an ack to the ACK tx Fifo
 *
 * @param nodeIdRecipient the ID of the node
 * @param counter the counter value of the ack
 */
void LoRaHomeGateway::putAck(uint8_t nodeIdRecipient, uint16_t counter)
{
  LORA_HOME_ACK ack_packet = {0};
  ack_packet.header.counter = counter;
  ack_packet.header.messageType = LH_MSG_TYPE_GW_ACK;
  ack_packet.header.networkID = this->network_id;
  ack_packet.header.nodeIdEmitter = LH_NODE_ID_GATEWAY;
  ack_packet.header.nodeIdRecipient = nodeIdRecipient;
  ack_packet.header.payloadSize = 0;
  ack_packet.crc16 = crc16_ccitt((uint8_t *) &ack_packet, sizeof(LORA_HOME_PACKET_HEADER));
  xQueueSend(tx_packet_queue, &ack_packet, 0);
}

/**
 * @brief enable message receiving on LoRa
 *
 */
void LoRaHomeGateway::enable()
{
  LoRa.onReceive(this->onReceive);
  LoRa.onTransmit(this->send);
}

/**
 * @brief disable message receiving on LoRa
 *
 */
void LoRaHomeGateway::disable()
{
  LoRa.onReceive(nullptr); // NULL before
  LoRa.onTransmit(nullptr);
}

/**
 * @brief Set Node in Rx Mode with disable invert IQ
 *
 * LoraWan reused principle to avoid node talking to each other
 * This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, and Node never reads messages from other Node
 */
void LoRaHomeGateway::rxMode()
{
  LoRa.disableInvertIQ(); // normal mode
  // put the radio into receive mode
  LoRa.receive();
}

/**
 * @brief Set Node in Tx Mode with enable invert IQ
 * LoraWan reused principle to avoid node talking to each other
 * This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, and Node never reads messages from other Node.
 */
void LoRaHomeGateway::txMode()
{
  LoRa.idle();           // set standby mode
  LoRa.enableInvertIQ(); // active invert I and Q signals
}

/**
 * @brief send a packet over Lora
 *
 */
void LoRaHomeGateway::send()
{

  uint8_t txBuffer[LH_FRAME_MAX_SIZE];
  BaseType_t anymsg = xQueueReceive(tx_packet_queue, txBuffer, 0);
  if (pdTRUE == anymsg)
  {
    tx_counter++;
    uint8_t size = LH_FRAME_HEADER_SIZE + txBuffer[LH_PACKET_INDEX_PAYLOAD_SIZE] + LH_FRAME_FOOTER_SIZE;
    txMode();
    LoRa.beginPacket();
    //char log[256] = "\0";
    for (uint8_t i = 0; i < size; i++)
    {
      LoRa.write(txBuffer[i]);
      //sprintf(log + strlen(log), "%02X:", txBuffer[i]);
    }
    LoRa.flush();
    LoRa.endPacket(false);
    rxMode();
    //serial_api_send_log_message(log);
  }
}

/**
 * @brief LoRa callback function when packets are available
 * Sort out the incoming LoRa messages in the right Queue for later processing (message, ack)
 * @param packet_size number of bytes available
 */
void LoRaHomeGateway::onReceive(int packet_size)
{
  // increment rx_counter - new message received
  rx_counter++;
  // invalid packet - flush rx fifo and return
  if ((packet_size > LH_FRAME_MAX_SIZE) || (packet_size < LH_FRAME_MIN_SIZE))
  {
    for (int i = 0; i < packet_size; i++)
    {
      LoRa.read();
    }
    return;
  }

  uint8_t rxMessage[LH_FRAME_MAX_SIZE];

  for (int i = 0; i < packet_size; i++)
  {
    rxMessage[i] = (uint8_t)LoRa.read();
  }

  if (!checkCRC(rxMessage, packet_size))
  {
    err_counter++;
    return;
  }

  LORA_HOME_PACKET *packet;
  packet = (LORA_HOME_PACKET *)&rxMessage[0];

  if ((packet->header.networkID == network_id) && ((packet->header.nodeIdRecipient == LH_NODE_ID_GATEWAY) || (packet->header.nodeIdRecipient == LH_NODE_ID_BROADCAST)))
  {
    // analyse the message type (ack or standard)
    switch (packet->header.messageType)
    {
    case LH_MSG_TYPE_NODE_MSG_ACK_REQ:
      lhg.putAck(packet->header.nodeIdEmitter, packet->header.counter);
      xQueueSend(rx_packet_queue, rxMessage, 0);
      break;
    case LH_MSG_TYPE_NODE_MSG_NO_ACK_REQ:
      xQueueSend(rx_packet_queue, rxMessage, 0);
      break;
    case LH_MSG_TYPE_NODE_ACK:
      if (packet_size != LH_FRAME_ACK_SIZE)
      {
        // ACK message length not valid
      }
      else
      {
        xQueueSend(rx_ack_packet_queue, rxMessage, 0);
      }
      break;
    case LH_MSG_TYPE_GW_ACK:
      // should not receive Gateway Ack ...
      break;
    default:
      // unknown message type. Shall be an error
      err_counter++;
      break;
    }
  }
}

/**
 * @brief compute CRC16 ccitt
 *
 * @param data data to be used to compute CRC16
 * @param data_len length of the buffer
 * @return uint16_t
 */
uint16_t LoRaHomeGateway::crc16_ccitt(const uint8_t *data, unsigned int data_len)
{
  uint16_t crc = 0xFFFF;

  if (data_len == 0)
    return 0;

  for (unsigned int i = 0; i < data_len; ++i)
  {
    uint16_t dbyte = data[i];
    crc ^= dbyte << 8;

    for (unsigned char j = 0; j < 8; ++j)
    {
      uint16_t mix = crc & 0x8000;
      crc = (crc << 1);
      if (mix)
      {
        crc = crc ^ 0x1021;
      }
    }
  }
  return crc;
}

/**
 * @brief check the CRC
 *
 * @param packet packet to be checked
 * @param length length of the packet
 * @return true
 * @return false
 */
bool LoRaHomeGateway::checkCRC(const uint8_t *packet, uint8_t length)
{
  // check CRC - last 2 bytes should contain CRC16
  uint8_t lowCRC = packet[length - 2];
  uint8_t highCRC = packet[length - 1];
  uint16_t rx_crc16 = lowCRC | (highCRC << 8);
  // compute CRC16 without the last 2 bytes
  uint16_t crc16 = crc16_ccitt(packet, length - 2);
  // if CRC16 not valid, ignore LoRa message
  if (rx_crc16 != crc16)
  {
    return false;
  }
  return true;
}

LoRaHomeGateway lhg;
