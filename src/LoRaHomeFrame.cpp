#include <LoRaHomeFrame.h>

// #define DEBUG_ESP_PORT Serial
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

/**
 * @brief Construct a new LoRaHomeFrame:: LoRaHomeFrame object
 * 
 */
LoRaHomeFrame::LoRaHomeFrame()
{
    this->networkID = 0;
    this->nodeIdEmitter = 0;
    this->nodeIdRecipient = 0;
    this->messageType = 0;
    this->jsonPayload[0] = '\0';
    this->counter = 0;
}

/**
 * @brief Construct a new LoRaHomeFrame object
 * 
 * @param nodeIdEmitter node ID of the emitter
 * @param nodeIdRecipient node ID of recipient
 * @param messageType LoRa Home message type
 * @param counter value of the counter
 */
LoRaHomeFrame::LoRaHomeFrame(uint16_t networkID, uint8_t nodeIdEmitter, uint8_t nodeIdRecipient, uint8_t messageType, uint16_t counter)
{
    this->networkID = networkID;
    this->nodeIdEmitter = nodeIdEmitter;
    this->nodeIdRecipient = nodeIdRecipient;
    this->messageType = messageType;
    this->jsonPayload[0] = '\0';
    this->counter = counter;
}

/**
 * @brief serialize a LoRaHomeFrame into the given txBuffer
 * the size of the txBuffer shall be large enough to welcome the LoRaHomeFrame
 * 
 * @param txBuffer 
 * @return uint8_t 
 */
uint8_t LoRaHomeFrame::serialize(uint8_t *txBuffer)
{
    DEBUG_MSG("LoRaHomeFrame::serialize\n");
    txBuffer[LH_FRAME_INDEX_EMITTER] = this->nodeIdEmitter;
    txBuffer[LH_FRAME_INDEX_RECIPIENT] = this->nodeIdRecipient;
    txBuffer[LH_FRAME_INDEX_MESSAGE_TYPE] = this->messageType;
    txBuffer[LH_FRAME_INDEX_NETWORK_ID] = static_cast<int8_t>(this->networkID & 0xff);
    txBuffer[LH_FRAME_INDEX_NETWORK_ID + 1] = static_cast<int8_t>((this->networkID >> 8)) & 0xff;
    txBuffer[LH_FRAME_INDEX_COUNTER] = static_cast<int8_t>(this->counter & 0xff);
    txBuffer[LH_FRAME_INDEX_COUNTER + 1] = static_cast<int8_t>((this->counter >> 8)) & 0xff;
    uint8_t payloadSize = strlen(this->jsonPayload);
    DEBUG_MSG("--- payload size %i\n", payloadSize);
    txBuffer[LH_FRAME_INDEX_PAYLOAD_SIZE] = payloadSize;
    if (payloadSize > 0)
    {
        memcpy((char *)&txBuffer[LH_FRAME_INDEX_PAYLOAD], this->jsonPayload, payloadSize);
    }
    this->crc16 = crc16_ccitt(txBuffer, LH_FRAME_HEADER_SIZE + payloadSize);
    txBuffer[LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE - 2] = this->crc16 & 0xff;
    txBuffer[LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE - 1] = (this->crc16 >> 8) & 0xff;
    return LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE;
}

/**
 * @brief 
 * 
 * @param rawBytesWithCRC 
 * @param length 
 * @return true 
 * @return false 
 */
bool LoRaHomeFrame::checkCRC(const uint8_t *rawBytesWithCRC, uint8_t length)
{
    DEBUG_MSG("LoRaHomeFrame::checkCRC\n");
    // check if packet is potentially valid. At least 11 bytes
    if (length < LH_FRAME_MIN_SIZE)
    {
        DEBUG_MSG("--- bad packet received too small\n");
        return false;
    }
    if (length > LH_FRAME_MAX_SIZE)
    {
        DEBUG_MSG("--- bad packet received too big\n");
        return false;
    }
    // check CRC - last 2 bytes should contain CRC16
    uint8_t lowCRC = rawBytesWithCRC[length - 2];
    uint8_t highCRC = rawBytesWithCRC[length - 1];
    uint16_t rx_crc16 = lowCRC | (highCRC << 8);
    DEBUG_MSG("--- Low CRC16 = %i\n", lowCRC);
    DEBUG_MSG("--- High CRC16 = %i\n", highCRC);
    // compute CRC16 without the last 2 bytes
    uint16_t crc16 = crc16_ccitt(rawBytesWithCRC, length - 2);
    // if CRC16 not valid, ignore LoRa message
    if (rx_crc16 != crc16)
    {
        DEBUG_MSG("--- CRC Error\n");
        return false;
    }
    DEBUG_MSG("--- valid CRC\n");
    return true;
}

/**
 * @brief Create a LoRaHomeFrame from a raw bytes message
 * 
 * @param rawBytesWithCRC raw bytes message with CRC included
 * @param length length of the message (number of bytes)
 * @param checkCRC indicate whether the CRC should be checked or not
 * 
 * @return true 
 * @return false 
 */
bool LoRaHomeFrame::createFromRxMessage(uint8_t *rawBytesWithCRC, uint8_t length, bool checkCRC)
{
    DEBUG_MSG("LoRaHomeFrame::createFromRxMessage\n");
    if (checkCRC)
    {
        if (!this->checkCRC(rawBytesWithCRC, length))
        {
            return false;
        }
    }
    // TODO should check whether data are valid or not?
    this->networkID = rawBytesWithCRC[LH_FRAME_INDEX_NETWORK_ID] | (rawBytesWithCRC[LH_FRAME_INDEX_NETWORK_ID + 1] << 8);
    DEBUG_MSG("--- network ID = %i\n", this->networkID);
    this->nodeIdEmitter = rawBytesWithCRC[LH_FRAME_INDEX_EMITTER];
    this->nodeIdRecipient = rawBytesWithCRC[LH_FRAME_INDEX_RECIPIENT];
    this->messageType = rawBytesWithCRC[LH_FRAME_INDEX_MESSAGE_TYPE];
    this->counter = rawBytesWithCRC[LH_FRAME_INDEX_COUNTER] | (rawBytesWithCRC[LH_FRAME_INDEX_COUNTER + 1] << 8);
    this->payloadSize = rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD_SIZE];
    DEBUG_MSG("--- payload size = %i\n", this->payloadSize);
    if (this->payloadSize > LH_FRAME_MAX_PAYLOAD_SIZE)
    {
        DEBUG_MSG("--- invalid payload size\n");
        return false;
    }
    // copy the json payload if any
    if (this->payloadSize != 0)
    {
        memcpy(this->jsonPayload, &rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD], rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD_SIZE]);
        this->jsonPayload[this->payloadSize] = '\0';
    }
    return true;
}

/**
 * @brief compute CRC16 ccitt
 * 
 * @param data data to be used to compute CRC16
 * @param data_len length of the buffer
 * @return uint16_t 
 */
uint16_t LoRaHomeFrame::crc16_ccitt(const uint8_t *data, unsigned int data_len)
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