#ifndef LORA_HOME_CONFIGURATION_H
#define LORA_HOME_CONFIGURATION_H

// -------------------------------------------------------
// LoRa HARDWARE CONFIGURATION 
// (SS = 18, RST = 14, DIO = 26 for Heltec LoRa v2 board)
// -------------------------------------------------------
//define the pins used by the transceiver module
#define SS 18
#define RST 14
#define DIO0 26

// -------------------------------------------------------
// LoRa MODEM SETTINGS
// -------------------------------------------------------
// The sync word assures you don't get LoRa messages from other LoRa transceivers
// ranges from 0-0xFF - make sure that the node is using the same sync word
#define LORA_SYNC_WORD 0xB2

// frequency
// can be changed to 433E6, 915E6
#define LORA_FREQUENCY 868E6
//#define LORA_FREQUENCY 867990000 // for FRED board

// change the spreading factor of the radio.
// LoRa sends chirp signals, that is the signal frequency moves up or down, and the speed moved is roughly 2**spreading factor.
// Each step up in spreading factor doubles the time on air to transmit the same amount of data.
// Higher spreading factors are more resistant to local noise effects and will be read more reliably at the cost of lower data rate and more congestion.
// Supported values are between 7 and 12
#define LORA_SPREADING_FACTOR 7  // 10 for Venon School, else 7

// LoRa signal bandwidth
// Bandwidth is the frequency range of the chirp signal used to carry the baseband data.
// Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3
#define LORA_SIGNAL_BANDWIDTH 125E3  // 250E3 for Venon School, else 125E3

// Coding rate of the radio
// LoRa modulation also adds a forward error correction (FEC) in every data transmission.
// This implementation is done by encoding 4-bit data with redundancies into 5-bit, 6-bit, 7-bit, or even 8-bit.
// Using this redundancy will allow the LoRa signal to endure short interferences.
// The Coding Rate (CR) value need to be adjusted according to conditions of the channel used for data transmission.
// If there are too many interference in the channel, then it’s recommended to increase the value of CR.
// However, the rise in CR value will also increase the duration for the transmission
// Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. The coding rate numerator is fixed at 4
#define LORA_CODING_RATE_DENOMINATOR 5


#endif 