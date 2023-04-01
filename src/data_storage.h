#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include "lora_home_configuration.h"

class DataStorage
{
public:
    DataStorage();
    void init();
    void load_configuration();
    LORA_CONFIGURATION get_lora_configuration();
    uint16_t get_lora_home_network_id();
    void set_lora_home_network_id(uint16_t value);
    void set_lora_configuration(LORA_CONFIGURATION *lc);

private:
    void save_configuration();

};

extern DataStorage data_storage;

#endif
