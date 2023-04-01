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
    void set_lora_configuration(LORA_CONFIGURATION *lc);
    void save_lora_configuration();
private:

};

extern DataStorage data_storage;

#endif
