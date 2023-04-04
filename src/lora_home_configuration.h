/**
 * @file lora_home_configuration.h
 * @author mchacher
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef LORA_HOME_CONFIGURATION_H
#define LORA_HOME_CONFIGURATION_H

/**
 * @brief LoRa HARDWARE CONFIGURATION
 * (SS = 18, RST = 14, DIO = 26 for Heltec LoRa v2 board)
 * define the pins used by the transceiver module
 * 
 */
#define SS 18
#define RST 14
#define DIO0 26

/**
 * @brief Lora frequency channels
 * 3 channels are defined. CH 3 being the default one.
 * CH_1 867.4 MHz
 * CH_2 867.7 MHz
 * CH_3 868.0 MHz
 */
enum Lora_Frequency_Channel
{
    CH_NONE = 0,
    CH_1 = 867400000,
    CH_2 = 867700000,
    CH_3 = 868000000
};

/**
 * @brief LoRa signal bandwidth
 * Bandwidth is the frequency range of the chirp signal used to carry the baseband data.
 * Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3
 * 250E3 for Venon School, else 125E3
 * 
 */
enum Lora_Signal_Bandwidth
{
    
    BW_NONE = 0,
    BW_7_8KHZ = 7800,
    BW_10_4KHZ = 10400,
    BW_15_6KHZ = 15600,
    BW_20_8KHZ = 20800,
    BW_31_25KHZ = 31250,
    BW_41_7KHZ = 41700,
    BW_62_5KHZ = 62500,
    BW_125KHZ = 125000,
    BW_250KHZ = 250000
};

/**
 * @brief change the spreading factor of the radio.
 * LoRa sends chirp signals, that is the signal frequency moves up or down, and the speed moved is roughly 2**spreading factor.
 * Each step up in spreading factor doubles the time on air to transmit the same amount of data.
 * Higher spreading factors are more resistant to local noise effects and will be read more reliably at the cost of lower data rate and more congestion.
 * Supported values are between 7 and 12
 * 10 for Venon School, else 7
 * 
 */
enum Lora_Spreading_Factor
{
    SF_NONE = 0,
    SF_7 = 7,
    SF_8 = 8,
    SF_9 = 9,
    SF_10 = 10,
    SF_11 = 11,
    SF_12 = 12
};

/**
 * @brief Coding rate of the radio
 * LoRa modulation also adds a forward error correction (FEC) in every data transmission.
 * This implementation is done by encoding 4-bit data with redundancies into 5-bit, 6-bit, 7-bit, or even 8-bit.
 * Using this redundancy will allow the LoRa signal to endure short interferences.
 * The Coding Rate (CR) value need to be adjusted according to conditions of the channel used for data transmission.
 * If there are too many interference in the channel, then it’s recommended to increase the value of CR.
 * However, the rise in CR value will also increase the duration for the transmission
 * Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. The coding rate numerator is fixed at 4
 * 
 */
enum Lora_Coding_Rate
{
    CR_NONE = 0,
    CR_4 = 4,
    CR_5 = 5,
    CR_6 = 6,
    CR_7 = 8,
    CR_8 = 8
};

typedef struct
{
    enum Lora_Frequency_Channel channel;
    enum Lora_Signal_Bandwidth bandwidth;
    enum Lora_Spreading_Factor spreading_factor;
    enum Lora_Coding_Rate coding_rate;
} LORA_CONFIGURATION;

#endif