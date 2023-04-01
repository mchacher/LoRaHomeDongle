#include <Arduino.h>
#include <Preferences.h>
#include "data_storage.h"
#include "lora_home_configuration.h"

/**
 * @brief name of the DATA_ZONE used for the TTN_HiveScale
 *
 */
const char *DATA_ZONE = "loradongle";

Preferences prefs;

uint32_t bandwidth = 0;
uint32_t spreading_factor = 0;
uint32_t coding_rate = 0;

const char *KEY_CH = "K_CH";
const char *KEY_BW = "K_BW";
const char *KEY_SF = "K_SF";
const char *KEY_CR = "K_CR";

LORA_CONFIGURATION lora_config = {
    .channel = CH_NONE,
    .bandwidth = BW_NONE,
    .spreading_factor = SF_NONE,
    .coding_rate = CR_NONE};

const LORA_CONFIGURATION lora_default_config = {
    .channel = CH_3,
    .bandwidth = BW_125KHZ,
    .spreading_factor = SF_7,
    .coding_rate = CR_5};

DataStorage::DataStorage()
{
}

/**
 * @brief setup persistent data management while reserving a data zone in NSV
 *
 */
void DataStorage::init()
{
    prefs.begin(DATA_ZONE);
}

/**
 * @brief 
 *
 */
void DataStorage::load_configuration()
{

    Lora_Frequency_Channel ch = (Lora_Frequency_Channel)prefs.getUInt(KEY_CH, 0);
    if (ch != 0)
    {
        lora_config.channel = ch;
    }
    else
    {
        lora_config.channel = lora_default_config.channel;
    }
    Lora_Signal_Bandwidth bw = (Lora_Signal_Bandwidth)prefs.getUInt(KEY_BW, 0);
    if (bw != 0)
    {
        lora_config.bandwidth = bw;
    }
    else
    {
        lora_config.bandwidth = lora_default_config.bandwidth;
    }
    
    Lora_Spreading_Factor sf = (Lora_Spreading_Factor)prefs.getUInt(KEY_SF, 0);
    if (sf != 0)
    {
        lora_config.spreading_factor = sf;
    }
    else
    {
        lora_config.spreading_factor = lora_default_config.spreading_factor;
    }
    Lora_Coding_Rate cr = (Lora_Coding_Rate)prefs.getUInt(KEY_CR, 0);
    if (cr != 0)
    {
        lora_config.coding_rate = cr;
    }
    else
    {
        lora_config.coding_rate = lora_default_config.coding_rate;
    }
}

void DataStorage::save_lora_configuration()
{
    prefs.putUInt(KEY_CH, lora_config.channel);
    prefs.putUInt(KEY_BW, lora_config.bandwidth);
    prefs.putUInt(KEY_SF, lora_config.spreading_factor);
    prefs.putUInt(KEY_CR, lora_config.coding_rate);
}


void DataStorage::set_lora_configuration(LORA_CONFIGURATION *lc)
{
    lora_config.channel = lc->channel;
    lora_config.bandwidth = lc->bandwidth;
    lora_config.spreading_factor = lc->spreading_factor;
    lora_config.coding_rate = lc->coding_rate;
    this->save_lora_configuration();
}


/**
 * @brief setup persistent data management while reserving a datasone in NSV
 *
 */
LORA_CONFIGURATION DataStorage::get_lora_configuration()
{
    if ((lora_config.channel != CH_NONE) && (lora_config.bandwidth != BW_NONE) && (lora_config.spreading_factor != SF_NONE) && (lora_config.coding_rate != CR_NONE))
    {
        return lora_config;
    }
    return lora_default_config;
}