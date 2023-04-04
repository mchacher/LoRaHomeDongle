/**
 * @file data_storage.cpp
 * @author mchacher
 * @brief  manage persistency
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Arduino.h>
#include <Preferences.h>
#include "data_storage.h"
#include "lora_home_configuration.h"

/**
 * @brief name of the DATA_ZONE
 *
 */
const char *DATA_ZONE = "loradongle";

Preferences prefs;

uint32_t bandwidth = 0;
uint32_t spreading_factor = 0;
uint32_t coding_rate = 0;

/**
 * @brief key - frequency channel
 * 
 */
const char *KEY_CH = "K_CH";

/**
 * @brief key - signal bandwidth
 * 
 */
const char *KEY_BW = "K_BW";

/**
 * @brief key - spreading factor
 * 
 */
const char *KEY_SF = "K_SF";

/**
 * @brief key - coding rate
 * 
 */
const char *KEY_CR = "K_CR";

/**
 * @brief key - network id
 * 
 */
const char *KEY_NID = "K_NID";

/**
 * @brief lora configuration
 * 
 */
LORA_CONFIGURATION lora_config = {
    .channel = CH_NONE,
    .bandwidth = BW_NONE,
    .spreading_factor = SF_NONE,
    .coding_rate = CR_NONE};

/**
 * @brief defaut lora configuration when no persisdent data available
 * 
 */
const LORA_CONFIGURATION lora_default_config = {
    .channel = CH_3,
    .bandwidth = BW_125KHZ,
    .spreading_factor = SF_7,
    .coding_rate = CR_5};

/**
 * @brief lora home network id
 * 
 */
uint16_t lora_home_network_id = 0;

/**
 * @brief lora home default network id, rock and roll
 * 
 */
const uint16_t default_network_id = 0xACDC;

/**
 * @brief Construct a new Data Storage:: Data Storage object
 * 
 */
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
 * @brief load dongle configuration in NSV. Lora settings and Lora  Home Network ID
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
    uint16_t n_id = prefs.getUShort(KEY_NID, 0);
    if (n_id != 0)
    {
        lora_home_network_id = n_id;
    }
    else
    {
        lora_home_network_id = default_network_id;
    }
}

/**
 * @brief assessor
 * 
 * @return uint16_t lora home network id
 */
uint16_t DataStorage::get_lora_home_network_id()
{
    return lora_home_network_id;
}

/**
 * @brief set and save to persistent memory
 * 
 * @param value lora home network id
 */
void DataStorage::set_lora_home_network_id(uint16_t value)
{
    lora_home_network_id = value;
    this->save_configuration();
}

/**
 * @brief save actual configuration in persistent memory (NSV)
 * 
 */
void DataStorage::save_configuration()
{
    prefs.putUInt(KEY_CH, lora_config.channel);
    prefs.putUInt(KEY_BW, lora_config.bandwidth);
    prefs.putUInt(KEY_SF, lora_config.spreading_factor);
    prefs.putUInt(KEY_CR, lora_config.coding_rate);
    prefs.putUShort(KEY_NID, lora_home_network_id);
}

/**
 * @brief set lora communication settings paramenters
 * save them to persistent memory
 * 
 * @param lc 
 */
void DataStorage::set_lora_configuration(LORA_CONFIGURATION *lc)
{
    lora_config.channel = lc->channel;
    lora_config.bandwidth = lc->bandwidth;
    lora_config.spreading_factor = lc->spreading_factor;
    lora_config.coding_rate = lc->coding_rate;
    this->save_configuration();
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