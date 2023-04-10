/**
 * @file main.cpp
 * @author mchacher
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <Arduino.h>
#include <esp_system.h>
#include "lora_home_gateway.h"
#include "display.h"
#include "uart.h"
#include "serial_api.h"
#include "dongle_configuration.h"
#include "lora_home_configuration.h"
#include "data_storage.h"
#include "version.h"

// uncomment to activate the watchdog
#define WATCHDOG
// watchdog management
#ifdef WATCHDOG
// time in ms to trigger the watchdog
const int wdtTimeout = 60000;
hw_timer_t *timer = NULL;
#endif

// timeout for refreshing display
#define DISPLAY_TIMEOUT_REFRESH 1000 // 1s

// Display
Display display(lhg);
DataStorage data_storage;

// Tasks and Timers
TaskHandle_t taskHandle = NULL;
TimerHandle_t xTimerDisplayRefresh = NULL;

#ifdef WATCHDOG

/**
 * @brief watchdog Callback
 * restart esp if triggered
 */
void IRAM_ATTR resetModule()
{
  esp_restart();
}
#endif

/**
 * @brief FreeRTOS task
 * process incoming system packets on the UART
 * @param pvParameters not used
 */
void task_sys_dongle(void *pvParameters)
{
  SERIAL_PACKET *serial_packet;
  uint8_t rx_buffer[256];
  while (1)
  {
    if (serial_api_get_sys_dongle_packet(rx_buffer))
    {
      serial_packet = (SERIAL_PACKET *)rx_buffer;
      DONGLE_SYS_PACKET *sys_packet;
      sys_packet = (DONGLE_SYS_PACKET *)serial_packet->data;
      LORA_CONFIGURATION *lc;
      // char buffer[256] = "\nTask_sys_dongle:";
      switch (sys_packet->sys_type)
      {
      case TYPE_SYS_SET_LORA_SETTINGS:
        lc = (LORA_CONFIGURATION *)sys_packet->payload;
        data_storage.set_lora_configuration(lc);
        // sprintf(buffer + strlen(buffer), "\n - Channel = %i", lc->channel);
        // sprintf(buffer + strlen(buffer), "\n - Bandwidth = %i", lc->bandwidth);
        // sprintf(buffer + strlen(buffer), "\n - Coding Rage = %i", lc->coding_rate);
        // sprintf(buffer + strlen(buffer), "\n - Spreading Factor = %i", lc->spreading_factor);
        // serial_api_send_log_message(buffer);
        lhg.disable();
        lhg.setup(lc, data_storage.get_lora_home_network_id());
        break;
      case TYPE_SYS_RESET:
        esp_restart();
        break;
      case TYPE_SYS_GET_ALL_SETTINGS:
        // sprintf(buffer + strlen(buffer), " sending all settings");
        // serial_api_send_log_message(buffer);
        DONGLE_ALL_SETTINGS_PACKET_PAYLOAD packet_settings;
        packet_settings.version_major = VERSION_MAJOR;
        packet_settings.version_minor = VERSION_MINOR;
        packet_settings.version_patch = VERSION_PATCH;
        packet_settings.lora_config = data_storage.get_lora_configuration();
        packet_settings.lora_home_network_id = data_storage.get_lora_home_network_id();
        DONGLE_SYS_PACKET packet;
        packet.sys_type = TYPE_SYS_INFO_ALL_SETTINGS;
        memcpy(packet.payload, &packet_settings, sizeof(DONGLE_ALL_SETTINGS_PACKET_PAYLOAD));
        serial_api_send_sys_packet((uint8_t *)&packet, +sizeof(packet.sys_type) + sizeof(DONGLE_ALL_SETTINGS_PACKET_PAYLOAD));
        break;
      case TYPE_SYS_SET_LORA_HOME_NETWORK_ID:
        uint16_t *value = (uint16_t *)sys_packet->payload;
        // sprintf(buffer + strlen(buffer), " network_id = %02x", *value);
        // serial_api_send_log_message(buffer);
        data_storage.set_lora_home_network_id(*value);
        lhg.setNetworkID(*value);
        break;
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief FreeRTOS task
 * forward lora home packet received on the UART on the air
 * @param pvParameters not used
 */
void task_lora_home_send(void *pvParameters)
{
  SERIAL_PACKET *serial_packet; // Buffer to hold received messages
  LORA_HOME_PACKET *lora_packet;
  uint8_t rx_buffer[256];
  while (1)
  {
    if (serial_api_get_lora_home_packet(rx_buffer))
    {
      serial_packet = (SERIAL_PACKET *)rx_buffer;
      lora_packet = (LORA_HOME_PACKET *)serial_packet->data;
      lhg.putPacket((uint8_t *)lora_packet);
      // char buffer[256] = "\0";
      // for (int i = 0; i < serial_packet->header.data_length + sizeof(SERIAL_PACKET_HEADER); i++)
      // {
      //   sprintf(buffer + strlen(buffer), "%02x:", rx_buffer[i]);
      // }
      // serial_api_send_log_message(buffer);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief FreeRTOS task
 * check whether lora packets are available and forward them through the UART
 *
 * @param pvParameters not used
 */
void task_lora_home_receive(void *pvParameters)
{
  uint8_t packet[LH_FRAME_MAX_SIZE];
  while (1)
  {
    if (lhg.popLoRaHomePayload(packet))
    {
#ifdef WATCHDOG
      timerWrite(timer, 0);
#endif
      LORA_HOME_PACKET *lhp = (LORA_HOME_PACKET *)packet;
      uint8_t size = sizeof(LORA_HOME_PACKET_HEADER) + lhp->header.payloadSize; // + LH_FRAME_FOOTER_SIZE;
      serial_api_send_lora_home_packet(packet, size);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief FreeRTOS timer (every 1s)
 * refresh display each time is called
 * send heartbeat every hertbeat period
 *
 * @param xTimer
 */
void timer_heartbeat(TimerHandle_t xTimer)
{
  static unsigned long time = 0;
  display.refresh();
  if (millis() - time > HEARTBEAT_PERIOD)
  {
    DONGLE_HEARTBEAT_PACKET_PAYLOAD packet_heartbeat;
    packet_heartbeat.err_counter = lhg.err_counter;
    packet_heartbeat.rx_counter = lhg.rx_counter;
    packet_heartbeat.tx_counter = lhg.tx_counter;
    DONGLE_SYS_PACKET packet_sys;
    packet_sys.sys_type = TYPE_SYS_HEARTBEAT;
    // packet_sys.payload = (uint8_t *)&packet_heartbeat;
    memcpy(packet_sys.payload, &packet_heartbeat, sizeof(DONGLE_HEARTBEAT_PACKET_PAYLOAD));
    serial_api_send_sys_packet((uint8_t *)&packet_sys, sizeof(DONGLE_HEARTBEAT_PACKET_PAYLOAD) + sizeof(packet_sys.sys_type));
    time = millis();
  }
}

/**
 * @brief setup function - perform init of each module
 *
 * Perform the init of the following modules
 * - Data storage
 * - Display
 * - UART
 * - LoRa
 */
void setup()
{
  data_storage.init();
  data_storage.load_configuration();
  display.init();
  display.showUsbStatus(false);
  display.showLoRaStatus(false);
  uart_init();
  serial_api_init();
  display.showUsbStatus(true);

#ifdef WATCHDOG
  // watchdog configuration
  timer = timerBegin(0, 80, true);                  // timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  // attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); // set time in us
  timerAlarmEnable(timer);                          // enable interrupt
#endif
  // LoRa initialization
  display.showLoRaStatus(false);
  LORA_CONFIGURATION lc = data_storage.get_lora_configuration();
  lhg.setup(&lc, data_storage.get_lora_home_network_id());
  display.showLoRaStatus(true);
  // create tasks
  xTaskCreate(task_uart_rx, "task_uart_rx", 2048, NULL, 1, NULL);
  xTaskCreate(task_uart_tx, "task_uart_tx", 2048, NULL, 1, NULL);
  xTaskCreate(task_lora_home_send, "task_lora_home_send", 2048, NULL, 1, NULL);
  xTaskCreate(task_lora_home_receive, "task_lora_home_receive", 2048, NULL, 1, NULL);
  xTaskCreate(task_sys_dongle, "task_sys_dongle", 2048, NULL, 1, NULL);
  xTimerDisplayRefresh = xTimerCreate("timer_heartbeat", pdMS_TO_TICKS(DISPLAY_TIMEOUT_REFRESH), pdTRUE, 0, timer_heartbeat);
  xTimerStart(xTimerDisplayRefresh, 0);
}

/**
 * @brief main loop of LoRaHome Dongle
 * do nothing
 *
 */
void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
