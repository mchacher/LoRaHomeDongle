#ifndef DISPLAY_H
#define DISPLAY_H

#include "LoRaHomeGateway.h"
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
    static uint32_t rxCounter;
    static uint32_t txCounter;
    static uint32_t errorCounter;
};

#endif