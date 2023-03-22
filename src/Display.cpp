#include <Display.h>
#include <Arduino.h>
#include <LoRaHomeGateway.h>
#include <SPI.h>
#include <U8x8lib.h>

// OLED Display pins configuration
#define SSD1306_CLK     15
#define SSD1306_DATA    4
#define SSD1306_RST     16

// should be move as a private variable of the class
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SSD1306_CLK, SSD1306_DATA, SSD1306_RST);

// display lines configuration
#define DISPLAY_LINE_HEADING        0
#define DISPLAY_LINE_STATUS_USB     1
#define DISPLAY_LINE_STATUS_LORA    2
#define DISPLAY_LINE_COUNTER_RX     4
#define DISPLAY_LINE_COUNTER_TX     5
#define DISPLAY_LINE_COUNTER_ERROR  6
#define DISPLAY_LINE_REFRESH_CURSOR 7

#define MAX_CHARACTERS_LINE 16

// lines fixed messages
const char* TXT_LORA2MQTT_HEADER = "LoRaHomeDongle";
const char* TXT_LORA_OK = "LoRa ... ok";
const char* TXT_LORA_WAIT = "LoRa ...";
const char* TXT_USB_OK = "Usb  ... ok";
const char* TXT_USB_WAIT = "Usb  ...";
const char* TXT_RX_COUNTER ="*Rx  ";
const char* TXT_TX_COUNTER ="*Tx  ";
const char* TXT_ERR_COUNTER ="*Err ";
const char* TXT_REFRESH_CURSOR = "-";

// rxCounter - each time a LoRa message is received, counter is incremented
uint32_t Display::rxCounter = 0;
// txCounter - each time a LoRa message is sent out, counter is incremented
uint32_t Display::txCounter = 0;
// errorCounter - each time an error is triggered
uint32_t Display::errorCounter = 0;

/**
 * @brief Construct a new Display:: Display object
 * 
 */
Display::Display(LoRaHomeGateway& lhg)
{
    this->lhg = &lhg;
}


/**
 * @brief referesh counters (Rx, Tx and Errors)
 *  
 */
void Display::refresh()
{
    static uint8_t cursor = 0;

    if (this->rxCounter != lhg->rxCounter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_RX);
        String msgRxCounter = TXT_RX_COUNTER;
        msgRxCounter += lhg->rxCounter;
        this->rxCounter = lhg->rxCounter;
        u8x8.drawString(0, DISPLAY_LINE_COUNTER_RX, msgRxCounter.c_str());
    }

    if (this->txCounter != lhg->txCounter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_TX);
        String msgTxCounter = TXT_TX_COUNTER;
        msgTxCounter += lhg->txCounter;
        this->txCounter = lhg->txCounter;
        u8x8.drawString(0, DISPLAY_LINE_COUNTER_TX, msgTxCounter.c_str());
    }

    if (this->errorCounter != lhg->errorCounter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_ERROR);
        String msgError = TXT_ERR_COUNTER;
        msgError += lhg->errorCounter;
        this->errorCounter = lhg->errorCounter;
        u8x8.drawString(0, DISPLAY_LINE_COUNTER_ERROR, msgError.c_str());
    }
    if (cursor > (MAX_CHARACTERS_LINE -1))
    {
        u8x8.clearLine(DISPLAY_LINE_REFRESH_CURSOR);
        cursor = 0;
    }
    u8x8.drawString(cursor, DISPLAY_LINE_REFRESH_CURSOR, TXT_REFRESH_CURSOR);
    cursor++; 
}

/**
 * @brief initialize display
 * 
 */
void Display::init()
{
    u8x8.begin();
    u8x8.setFont(u8x8_font_5x7_f);
    u8x8.drawString(0, DISPLAY_LINE_HEADING, TXT_LORA2MQTT_HEADER);
}


/**
 * @brief Display LoRa connection status
 * 
 * @param status Status of the LoRa connection
 */
void Display::showLoRaStatus(bool status)
{
    u8x8.clearLine(DISPLAY_LINE_STATUS_LORA);
    if (status)
    {
        u8x8.drawString(0, DISPLAY_LINE_STATUS_LORA, TXT_LORA_OK);
    }
    else
    {
        u8x8.drawString(0, DISPLAY_LINE_STATUS_LORA, TXT_LORA_WAIT);
    }
}

/**
 * @brief Display Usb connection status
 * 
 * @param status Status of the LoRa connection
 */
void Display::showUsbStatus(bool status)
{
    u8x8.clearLine(DISPLAY_LINE_STATUS_USB);
    if (status)
    {
        u8x8.drawString(0, DISPLAY_LINE_STATUS_USB, TXT_USB_OK);
    }
    else
    {
        u8x8.drawString(0, DISPLAY_LINE_STATUS_USB, TXT_USB_WAIT);
    }
}