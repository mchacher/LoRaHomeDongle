#ifndef LORAHOMEFRAME_H
#define LORAHOMEFRAME_H

#include <Arduino.h>

#define LH_FRAME_MAX_PAYLOAD_SIZE 128

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

typedef struct __attribute__((__packed__))
{
    LORA_HOME_PACKET_HEADER header;
    uint16_t crc16;
} LORA_HOME_ACK;


const uint8_t LH_FRAME_HEADER_SIZE = sizeof(LORA_HOME_PACKET_HEADER);
const uint8_t LH_FRAME_FOOTER_SIZE = 2; // only CRC for now
const uint8_t LH_FRAME_MIN_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_ACK_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_MAX_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE + LH_FRAME_MAX_PAYLOAD_SIZE;



const uint8_t LH_NODE_ID_GATEWAY = 0x00;
const uint8_t LH_NODE_ID_BROADCAST = 0xFF;

const uint8_t LH_MSG_TYPE_NODE_MSG_ACK_REQ = 0x01;
const uint8_t LH_MSG_TYPE_NODE_MSG_NO_ACK_REQ = 0x00;
const uint8_t LH_MSG_TYPE_GW_MSG_NO_ACK = 0x02;
const uint8_t LH_MSG_TYPE_GW_MSG_ACK = 0x03;
const uint8_t LH_MSG_TYPE_NODE_ACK = 0x04;
const uint8_t LH_MSG_TYPE_GW_ACK = 0x06;

const uint8_t LH_PACKET_INDEX_PAYLOAD_SIZE = 7; // 2 bytes

#endif