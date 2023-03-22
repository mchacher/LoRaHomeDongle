#include <LoRaHomeGateway.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include "esp_task_wdt.h"
#include "Configuration_LoRaHome.h"
#include "serial_api.h"

#define DEBUG_ESP_PORT Serial
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

// White LED management of heltec_wifi_lora_32_V2 board
#define LED_WHITE 25

#define ACK_TIMEOUT 300          // 200ms max to receive an Ack
#define MAX_RETRY_NO_VALID_ACK 3 // 3 retries max

// for each node sending data
// index = number of the node
// [index] = value of last Tx
uint8_t nodeTxCounterTable[255];

// -------------------------------------------------------
// LoRaHome CONFIGURATION
// -------------------------------------------------------

// incomig LoRa message queue
QueueHandle_t LoRaHomeGateway::rxMsgQueue = xQueueCreate(5, LH_FRAME_MAX_SIZE * sizeof(uint8_t));
// incoming LoRa ACK queue
QueueHandle_t LoRaHomeGateway::rxAckQueue = xQueueCreate(5, LH_FRAME_ACK_SIZE * sizeof(uint8_t));
// tx LoRa queue
QueueHandle_t LoRaHomeGateway::txQueue = xQueueCreate(5, LH_FRAME_MAX_SIZE * sizeof(uint8_t));

// if running on Core 1 - same as per Arduino Framework
// if running on Core 2 - leverage dual core architecture of ESP32
static int LoRaCore = 0;
TaskHandle_t taskHandlerLoRa;

// rxCounter - each time a LoRa message is received, counter is incremented
uint32_t LoRaHomeGateway::rxCounter = 0;
// txCounter - each time a LoRa message is sent out, counter is incremented
uint32_t LoRaHomeGateway::txCounter = 0;
// errorCounter - each time an error is triggered
uint32_t LoRaHomeGateway::errorCounter = 0;
// counter used inside Tx LoRaHomeFrame
uint16_t LoRaHomeGateway::lhfTxCounter = 0;
// lastMsgProcessedTimeStamp - use to feed watchdog. Each time a message is received
unsigned long LoRaHomeGateway::lastMsgProcessedTimeStamp = millis();

const char *JSON_KEY_NODE_NAME = "node";
const char *JSON_KEY_TX_COUNTER = "#tx";

/**
 * @brief Construct a new LoRaHomeGateway::LoRaHome object
 *
 */
LoRaHomeGateway::LoRaHomeGateway()
{
}

/**
 * @brief setup LoRaHome
 *
 */
void LoRaHomeGateway::setup()
{
  DEBUG_MSG("LoRaHomeGateway::setup\n");
  // configure Pinout for white LED
  pinMode(LED_WHITE, OUTPUT);
  // setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  DEBUG_MSG("--- LoRa.begin ... \n");
  while (!LoRa.begin(LORA_FREQUENCY))
  {
    delay(500);
  }
  DEBUG_MSG("--- LoRa.begin ... done\n");

  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE_DENOMINATOR);
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(LORA_SYNC_WORD);
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
      &taskHandlerLoRa, /* Task handler to keep track of created task */
      LoRaCore);        /* pin task to core 0, default Arduino setup and main running on core 1 */

  // set in rx mode.
  this->rxMode();
}

void LoRaHomeGateway::sendPacket(uint8_t *packet)
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
    xQueueSend(txQueue, raw_packet, 0);
    BaseType_t anymsg = xQueueReceive(rxAckQueue, ackBuffer, pdMS_TO_TICKS(ACK_TIMEOUT));
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
 * @brief send a LoRaHome message and validate it with ack
 * @param mqttJsonMsg
 *
 */
void LoRaHomeGateway::forwardMessageToNode(char *mqttJsonMsg)
{
  DEBUG_MSG("LoRaHomeGateway::forwardMessageToNode\n");
  uint8_t retry = 0;
  uint8_t ackBuffer[LH_FRAME_ACK_SIZE];
  // TODO check how to define the right size
  StaticJsonDocument<LH_MQTT_MSG_MAX_SIZE * 2> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, mqttJsonMsg);
  if (error)
  {
    DEBUG_MSG("--- mqtt message / deserialization error");
    return;
  }
  if (jsonDoc[JSON_KEY_NODE_NAME].isNull() == true)
  {
    DEBUG_MSG("--- invalid MQTT JSON message / node recipent node specified");
    return;
  }
  // create a LoRaHomeFrame
  LoRaHomeFrame lhf;
  uint8_t nodeIdRecipient = jsonDoc[JSON_KEY_NODE_NAME];
  uint8_t counter = lhfTxCounter++;
  lhf.networkID = MY_NETWORK_ID;
  lhf.nodeIdEmitter = LH_NODE_ID_GATEWAY;
  lhf.messageType = LH_MSG_TYPE_GW_MSG_ACK;
  lhf.nodeIdRecipient = nodeIdRecipient;
  lhf.counter = counter;
  jsonDoc.remove(JSON_KEY_NODE_NAME);
  serializeJson(jsonDoc, lhf.jsonPayload);
  bool success = false;
  // send the LoRaHome message and loop max retry if necessary
  do
  {
    DEBUG_MSG("--- send msg, retry = %i\n", retry);
    uint8_t txBuffer[LH_FRAME_MAX_SIZE];
    DEBUG_MSG("--- msg to forward: ");
    uint8_t size = lhf.serialize(txBuffer);
    for (uint8_t i = 0; i < size; i++)
    {
      DEBUG_MSG("%i ", txBuffer[i]);
    }
    DEBUG_MSG("\n");
    xQueueSend(txQueue, txBuffer, 0);

    unsigned long ackStartWaitingTime = millis();
    // loop until ACK_TIMEOUT expired or success
    // while (((millis() - ackStartWaitingTime) < ACK_TIMEOUT) && (success == false))
    // {
    // add 100ms delay to give back the hand to the IDLE task
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // TODO : réfléchir à la bonne valeur de timeout
    BaseType_t anymsg = xQueueReceive(rxAckQueue, ackBuffer, ACK_TIMEOUT / portTICK_PERIOD_MS);
    if (pdTRUE == anymsg)
    {
      DEBUG_MSG("--- ack received\n");
      LoRaHomeFrame lhfAck;
      lhfAck.createFromRxMessage(ackBuffer, LH_FRAME_ACK_SIZE, false);
      if ((lhfAck.nodeIdEmitter == nodeIdRecipient) && (lhfAck.counter == counter))
      {
        DEBUG_MSG("--- expected ack received\n");
        ackStartWaitingTime = millis() - ackStartWaitingTime;
        DEBUG_MSG("--- time to receive ack: %lu\n", ackStartWaitingTime);
        success = true;
      }
      else
      {
        DEBUG_MSG("--- ack received is not the one expected - add it back to the queue\n");
        // TODO: check what we should do to avoid losing an ack
        // if we add it to the back of the queue and it is the only one, next loop we will get it again ...$
        // on the other hand, since we pile tx requests, there should not be more than one Tx request at a time
      }
      // }
    }
    retry++;
    // loop if ack not received
  } while ((false == success) && (retry < MAX_RETRY_NO_VALID_ACK));
  DEBUG_MSG("--- exit forwardMessageToNode, retry = %i\n", retry);
}

/**
 * @brief pop the LoRaHomeFrame if any available in the Rx message queue
 *
 * @param lhf the LoRaHomeFrame reference to be used to return the message
 * @return true if a message was available
 * @return false if no message available
 */
bool LoRaHomeGateway::popLoRaHomePayload(uint8_t *rxBuffer)
{
  // uint8_t rxBuffer[LH_FRAME_MAX_SIZE];

  // any LoRa message in the queue?
  // enter critical section to retrieve data in the queue
  BaseType_t anymsg = xQueueReceive(rxMsgQueue, rxBuffer, 0);
  if (pdTRUE == anymsg)
  {
    DEBUG_MSG("LoRaHomeGateway::popLoRaHomePayload\n");
    lastMsgProcessedTimeStamp = millis();
    // frame has already been checked. So the message is valid, size can be computed
    // uint8_t size = LH_FRAME_HEADER_SIZE + rxBuffer[LH_FRAME_INDEX_PAYLOAD_SIZE] + LH_FRAME_FOOTER_SIZE;
    // lhf.createFromRxMessage(rxBuffer, size, false);
    return true;
  }
  return false;
}

/**
 * @brief send an ack to a given LoRaHome node
 *
 * @param nodeIdRecipient the ID of the node
 * @param counter the counter value of the ack
 */
void LoRaHomeGateway::sendAckToLoRaNode(uint8_t nodeIdRecipient, uint16_t counter)
{
  // DEBUG_MSG("LoRaHomeGateway::sendAckToLoRaNode\n");
  // DEBUG_MSG("--- recipient: %i\n", nodeIdRecipient);
  // DEBUG_MSG("--- counter: %i\n", counter);
  LoRaHomeFrame ackFrame(MY_NETWORK_ID, LH_NODE_ID_GATEWAY, nodeIdRecipient, LH_MSG_TYPE_GW_ACK, counter);
  uint8_t txBuffer[LH_FRAME_MAX_SIZE];
  ackFrame.serialize(txBuffer);
  xQueueSend(txQueue, txBuffer, 0);
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
 * LoraWan principle to avoid node talking to each other
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
 * LoraWan principle to avoid node talking to each other
 * This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, and Node never reads messages from other Node.
 */
void LoRaHomeGateway::txMode()
{
  LoRa.idle();           // set standby mode
  LoRa.enableInvertIQ(); // active invert I and Q signals
}

/**
 * @brief send a LoraHome Frame
 *
 * @param lhf the LoRaHome Frame to be sent
 */
void LoRaHomeGateway::send()
{

  uint8_t txBuffer[LH_FRAME_MAX_SIZE];
  BaseType_t anymsg = xQueueReceive(txQueue, txBuffer, 0);
  if (pdTRUE == anymsg)
  {
    txCounter++;
    uint8_t size = LH_FRAME_HEADER_SIZE + txBuffer[LH_FRAME_INDEX_PAYLOAD_SIZE] + LH_FRAME_FOOTER_SIZE;
    txMode();
    LoRa.beginPacket();
    // char log[256] = "\0";
    for (uint8_t i = 0; i < size; i++)
    {
      LoRa.write(txBuffer[i]);
      // sprintf(log + strlen(log), "%02X:", txBuffer[i]);
    }
    LoRa.flush();
    LoRa.endPacket(false);
    rxMode();
    // serial_api_send_log_message(log);
  }
}

/**
 * @brief LoRa callback function when packets are available
 * Sort out the incoming LoRa messages in the right Queue for later processing (message, ack)
 * @param packetSize number of bytes available
 */
void LoRaHomeGateway::onReceive(int packetSize)
{
  DEBUG_MSG("LoRaHomeGateway::onReceive: Packet Size = %i\n", packetSize);
  // increment rxCounter - new message received
  rxCounter++;
  // invalid packet - flush rx fifo and return
  if ((packetSize > LH_FRAME_MAX_SIZE) || (packetSize < LH_FRAME_MIN_SIZE))
  {
    for (int i = 0; i < packetSize; i++)
    {
      LoRa.read();
    }
    return;
  }

  uint8_t rxMessage[LH_FRAME_MAX_SIZE];

  for (int i = 0; i < packetSize; i++)
  {
    rxMessage[i] = (uint8_t)LoRa.read();
  }

  if (!checkCRC(rxMessage, packetSize))
  {
    DEBUG_MSG("--- Error: CRC NOT OK");
    errorCounter++;
    return;
  }

  LORA_HOME_PACKET *packet;
  packet = (LORA_HOME_PACKET *)&rxMessage[0];
  // DEBUG_MSG("--- Network ID: %X\n", packet->header.networkID);
  // DEBUG_MSG("--- node ID emitter: %i\n", packet->header.nodeIdEmitter);
  // DEBUG_MSG("--- node ID Recipient: %i\n", packet->header.nodeIdRecipient);
  // DEBUG_MSG("--- counter: %i\n", packet->header.counter);
  // DEBUG_MSG("--- message type: %i\n", packet->header.messageType);

  if ((packet->header.networkID == MY_NETWORK_ID) && ((packet->header.nodeIdRecipient == LH_NODE_ID_GATEWAY) || (packet->header.nodeIdRecipient == LH_NODE_ID_BROADCAST)))
  {
    // analyse the message type (ack or standard)
    switch (packet->header.messageType)
    {
    case LH_MSG_TYPE_NODE_MSG_ACK_REQ:
      // TODO MCH réactiver la gestion de l'ACK
      loraHomeGateway.sendAckToLoRaNode(packet->header.nodeIdEmitter, packet->header.counter);

      // if frame not already received (typically ack not received, and node resending)
      // add it to the queue, else drop it
      DEBUG_MSG("--- valid message requiring ACK, add it to the queue\n");
      xQueueSend(rxMsgQueue, rxMessage, 0);
      break;
    case LH_MSG_TYPE_NODE_MSG_NO_ACK_REQ:
      DEBUG_MSG("--- valid STANDARD message, add it to the queue\n");
      xQueueSend(rxMsgQueue, rxMessage, 0);
      break;
    case LH_MSG_TYPE_NODE_ACK:
      if (packetSize != LH_FRAME_ACK_SIZE)
      {
        // DEBUG_MSG("--- ACK message length not valid\n");
      }
      else
      {
        // DEBUG_MSG("--- ACK message, add it to the queue - msg\n");
        xQueueSend(rxAckQueue, rxMessage, 0);
      }
      break;
    case LH_MSG_TYPE_GW_ACK:
      // should not receive Gateway Ack ...
      break;
    default:
      // DEBUG_MSG("--- unknown message type. Shall be an error\n");
      errorCounter++;
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
 * @brief
 *
 * @param packet
 * @param length
 * @return true
 * @return false
 */
bool LoRaHomeGateway::checkCRC(const uint8_t *packet, uint8_t length)
{
  DEBUG_MSG("LoRaHomeFrame::checkCRC\n");
  // check CRC - last 2 bytes should contain CRC16
  uint8_t lowCRC = packet[length - 2];
  uint8_t highCRC = packet[length - 1];
  uint16_t rx_crc16 = lowCRC | (highCRC << 8);
  DEBUG_MSG("--- Low CRC16 = %i\n", lowCRC);
  DEBUG_MSG("--- High CRC16 = %i\n", highCRC);
  // compute CRC16 without the last 2 bytes
  uint16_t crc16 = crc16_ccitt(packet, length - 2);
  // if CRC16 not valid, ignore LoRa message
  if (rx_crc16 != crc16)
  {
    DEBUG_MSG("--- CRC Error\n");
    return false;
  }
  DEBUG_MSG("--- valid CRC\n");
  return true;
}

LoRaHomeGateway loraHomeGateway;
