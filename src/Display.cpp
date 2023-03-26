#include <Arduino.h>
#include <SPI.h>
#include <U8x8lib.h>
#include "display.h"
#include "lora_home_gateway.h"

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

// rx_counter - each time a LoRa message is received, counter is incremented
uint32_t Display::rx_counter = 0;
// tx_counter - each time a LoRa message is sent out, counter is incremented
uint32_t Display::tx_counter = 0;
// err_counter - each time an error is triggered
uint32_t Display::err_counter = 0;

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

    if (this->rx_counter != lhg->rx_counter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_RX);
        String msg_rx_counter = TXT_RX_COUNTER;
        msg_rx_counter += lhg->rx_counter;
        this->rx_counter = lhg->rx_counter;
        u8x8.drawString(0, DISPLAY_LINE_COUNTER_RX, msg_rx_counter.c_str());
    }

    if (this->tx_counter != lhg->tx_counter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_TX);
        String msg_tx_counter = TXT_TX_COUNTER;
        msg_tx_counter += lhg->tx_counter;
        this->tx_counter = lhg->tx_counter;
        u8x8.drawString(0, DISPLAY_LINE_COUNTER_TX, msg_tx_counter.c_str());
    }

    if (this->err_counter != lhg->err_counter)
    {
        u8x8.clearLine(DISPLAY_LINE_COUNTER_ERROR);
        String msgError = TXT_ERR_COUNTER;
        msgError += lhg->err_counter;
        this->err_counter = lhg->err_counter;
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
 * @brief Initialize display
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
 * @param status Show status of the LoRa connection
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
 * @param status Show status of the USB connection
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