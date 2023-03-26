#ifndef LORA_HOME_GATEWAY_H
#define LORA_HOME_GATEWAY_H

#include "lora_home_packet.h"

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
    static void onReceive(int packet_size);
    static void send();
    static bool checkCRC(const uint8_t *packet, uint8_t length);
    static uint16_t crc16_ccitt(const uint8_t *data, unsigned int data_len);

public:
    static uint32_t rx_counter;
    static uint32_t tx_counter;
    static uint32_t err_counter;
    static unsigned long last_packet_ts;

private:
    static QueueHandle_t rx_packet_queue;
    static QueueHandle_t rx_ack_packet_queue;
    static QueueHandle_t tx_packet_queue; 
    static uint16_t packet_id_counter;
};

extern LoRaHomeGateway lhg;

#endif