/**
 * @file display.h
 * @author mchacher
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "lora_home_gateway.h"
class Display
{
public:
    Display(LoRaHomeGateway &lhg);
    void init();
    void showAccessPointMode();
    void refresh();
    void showUsbStatus(bool status);
    void showLoRaStatus(bool status);

private:
    LoRaHomeGateway *lhg;
    static uint32_t rx_counter;
    static uint32_t tx_counter;
    static uint32_t err_counter;
};

#endif