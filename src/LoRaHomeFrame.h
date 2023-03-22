#ifndef LORAHOMEFRAME_H
#define LORAHOMEFRAME_H

#include <Arduino.h>

const uint8_t LH_FRAME_HEADER_SIZE = 8;
const uint8_t LH_FRAME_FOOTER_SIZE = 2; // only CRC for now. TODO: add security
const uint8_t LH_FRAME_MAX_PAYLOAD_SIZE = 128;
const uint8_t LH_FRAME_MIN_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_ACK_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_MAX_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE + LH_FRAME_MAX_PAYLOAD_SIZE;

const uint8_t LH_FRAME_INDEX_EMITTER = 0;
const uint8_t LH_FRAME_INDEX_RECIPIENT = 1;
const uint8_t LH_FRAME_INDEX_MESSAGE_TYPE = 2;
const uint8_t LH_FRAME_INDEX_NETWORK_ID = 3;
const uint8_t LH_FRAME_INDEX_COUNTER = 5;
const uint8_t LH_FRAME_INDEX_PAYLOAD_SIZE = 7; // 2 bytes
const uint8_t LH_FRAME_INDEX_PAYLOAD = 8;

const uint8_t LH_NODE_ID_GATEWAY = 0x00;
const uint8_t LH_NODE_ID_BROADCAST = 0xFF;

const uint8_t LH_MSG_TYPE_NODE_MSG_ACK_REQ = 0x01;
const uint8_t LH_MSG_TYPE_NODE_MSG_NO_ACK_REQ = 0x00;
// TODO not used. should be removed?
const uint8_t LH_MSG_TYPE_GW_MSG_NO_ACK = 0x02;
const uint8_t LH_MSG_TYPE_GW_MSG_ACK = 0x03;

const uint8_t LH_MSG_TYPE_NODE_ACK = 0x04;
const uint8_t LH_MSG_TYPE_GW_ACK = 0x06;

class LoRaHomeFrame
{
public:
    LoRaHomeFrame();
    LoRaHomeFrame(uint16_t networkID, uint8_t nodeIdEmitter, uint8_t nodeIdRecipient, uint8_t messageType, uint16_t counter);
    bool createFromRxMessage(uint8_t *rawBytesWithCRC, uint8_t length, bool checkCRC);
    // void createAck(uint8_t nodeIdEmitter, uint8_t nodeIdRecipient, uint16_t counter);
    uint8_t serialize(uint8_t *txBuffer);
    bool checkCRC(const uint8_t *rawBytesWithCRC, uint8_t length);

private:
    static uint16_t crc16_ccitt(const uint8_t *data, unsigned int data_len);

public:
    uint8_t nodeIdEmitter;
    uint8_t nodeIdRecipient;
    uint16_t networkID;
    uint8_t messageType;
    uint16_t counter;
    uint8_t payloadSize;
    uint16_t crc16;
    char jsonPayload[LH_FRAME_MAX_PAYLOAD_SIZE];
};

typedef struct __attribute__((__packed__))
{
    uint8_t nodeIdEmitter;
    uint8_t nodeIdRecipient;
    uint8_t messageType;
    uint16_t networkID;
    uint16_t counter;
    uint8_t payloadSize;
} LORA_HOME_PACKET_HEADER;

typedef struct __attribute__((__packed__))
{
    LORA_HOME_PACKET_HEADER header;
    uint8_t json_payload[LH_FRAME_MAX_PAYLOAD_SIZE];
    uint16_t crc16;
} LORA_HOME_PACKET;

#endif