#ifndef LORAHOMEGATEWAY_H
#define LORAHOMEGATEWAY_H

#include <LoRaHomeFrame.h>

const uint8_t LH_MQTT_MSG_MAX_SIZE = 128; // to align with MQTT_MAX_PACKET_SIZE in PubSubClient 

extern const char *JSON_KEY_NODE_NAME;
extern const char *JSON_KEY_TX_COUNTER;

class LoRaHomeGateway
{

public:
    LoRaHomeGateway();
    void setup();
    void sendPacket(uint8_t *packet);
    void forwardMessageToNode(char *mqttJsonMsg);
    //bool popLoRaHomePayload(LoRaHomeFrame& lhp);
    bool popLoRaHomePayload(uint8_t *rxBuffer);
    void sendAckToLoRaNode(uint8_t nodeIdRecipient, uint16_t counter);
    void enable();
    void disable();


private:
    static void rxMode();
    static void txMode();
    static void onReceive(int packetSize);
    static void send();
    static bool checkCRC(const uint8_t *packet, uint8_t length);
    static uint16_t crc16_ccitt(const uint8_t *data, unsigned int data_len);

public:
    static uint32_t rxCounter;
    static uint32_t txCounter;
    static uint32_t errorCounter;
    static unsigned long lastMsgProcessedTimeStamp;

private:
    static QueueHandle_t rxMsgQueue;
    static QueueHandle_t rxAckQueue;
    static QueueHandle_t txQueue; 
    static uint16_t lhfTxCounter;
};

extern LoRaHomeGateway loraHomeGateway;

#endif