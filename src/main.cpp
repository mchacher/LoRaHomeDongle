#include <Arduino.h>
#include <esp_system.h>
#include "lora_home_gateway.h"
#include "display.h"
#include "uart.h"
#include "serial_api.h"
#include "dongle_configuration.h"
#include "lora_home_configuration.h"
#include "data_storage.h"


// #define DEBUG_ESP_PORT Serial
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

// uncomment to activate the watchdog
#define WATCHDOG
// watchdog management
#ifdef WATCHDOG
// time in ms to trigger the watchdog
const int wdtTimeout = 10000;
hw_timer_t *timer = NULL;
#endif

// timeout for refreshing display
#define DISPLAY_TIMEOUT_REFRESH 1000 // 1s

// Display
Display display(lhg);

// Tasks and Timers
TaskHandle_t taskHandle = NULL;
TimerHandle_t xTimerDisplayRefresh = NULL;

//
DataStorage data_storage;

#ifdef WATCHDOG
/**
 * brief watchdog Callback
 *
 * restart module if called
 *
 */
void IRAM_ATTR resetModule()
{
  DEBUG_MSG("Watchdog triggering reboot\n");
  esp_restart();
}
#endif

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
        lhg.setup(lc);
        break;
      case TYPE_SYS_RESET:
        esp_restart();
        break;
      case TYPE_SYS_GET_ALL_SETTINGS:
        // sprintf(buffer + strlen(buffer), " sending all settings");
        // serial_api_send_log_message(buffer);
        DONGLE_ALL_SETTINGS_PACKET packet_settings;
        packet_settings.version_major = VERSION_MAJOR;
        packet_settings.version_minor = VERSION_MINOR;
        packet_settings.version_patch = VERSION_PATCH;
        packet_settings.lora_config = data_storage.get_lora_configuration();
        DONGLE_SYS_PACKET packet;
        packet.sys_type = TYPE_SYS_INFO_ALL_SETTINGS;
        memcpy(packet.payload, &packet_settings, sizeof(DONGLE_ALL_SETTINGS_PACKET));
        serial_api_send_sys_packet((uint8_t *)&packet, + sizeof(packet.sys_type) + sizeof(DONGLE_ALL_SETTINGS_PACKET));
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * brief task_lora_home_send
 *
 * forward lora home packet received over the UART on the air
 *
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
      lhg.sendPacket((uint8_t *)lora_packet);
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

void task_lora_home_receive(void *pvParameters)
{
  uint8_t packet[LH_FRAME_MAX_SIZE];
  while (1)
  {
    if (lhg.popLoRaHomePayload(packet))
    {
      LORA_HOME_PACKET *lhp = (LORA_HOME_PACKET *)packet;
      uint8_t size = sizeof(LORA_HOME_PACKET_HEADER) + lhp->header.payloadSize; // + LH_FRAME_FOOTER_SIZE;
      serial_api_send_lora_home_packet(packet, size);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief refresh display every DISPLAY_TIMEOUT_REFRESH
 */
void timer_hearbeat(TimerHandle_t xTimer)
{
  static unsigned long time = 0;
#ifdef WATCHDOG
  timerWrite(timer, 0);
#endif
  display.refresh();
  if (millis() - time > HEARBEAT_PERIOD)
  {
    DONGLE_HEARTBEAT_PACKET packet_heartbeat;
    packet_heartbeat.err_counter = lhg.err_counter;
    packet_heartbeat.rx_counter = lhg.rx_counter;
    packet_heartbeat.tx_counter = lhg.tx_counter;
    DONGLE_SYS_PACKET packet_sys;
    packet_sys.sys_type = TYPE_SYS_HEARTBEAT;
    // packet_sys.payload = (uint8_t *)&packet_heartbeat;
    memcpy(packet_sys.payload, &packet_heartbeat, sizeof(DONGLE_HEARTBEAT_PACKET));
    serial_api_send_sys_packet((uint8_t *)&packet_sys, sizeof(DONGLE_HEARTBEAT_PACKET) + sizeof(packet_sys.sys_type));
    time = millis();
  }
}

/**
 * @brief setup function - perform init of each module
 *
 * Perform the init of the following modules
 * - Serial port if debug is activated
 * - Display
 * - LoRa
 */
void setup()
{
  //
  data_storage.init();
  data_storage.load_configuration();
  // data_storage.save_lora_configuration();
  //  Display initialization
  display.init();
  // UART initialization
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
  lhg.setup(&lc);
  DEBUG_MSG("--- LoRa Init OK!\n");
  display.showLoRaStatus(true);
  DEBUG_MSG("Main Loop starting, run on Core %i\n\n", xPortGetCoreID());
  xTaskCreate(task_uart_rx, "task_uart_rx", 2048, NULL, 1, NULL);
  xTaskCreate(task_uart_tx, "task_uart_tx", 2048, NULL, 1, NULL);
  xTaskCreate(task_lora_home_send, "task_lora_home_send", 2048, NULL, 1, NULL);
  xTaskCreate(task_lora_home_receive, "task_lora_home_receive", 2048, NULL, 1, NULL);
  xTaskCreate(task_sys_dongle, "task_sys_dongle", 2048, NULL, 1, NULL);
  xTimerDisplayRefresh = xTimerCreate("timer_hearbeat", pdMS_TO_TICKS(DISPLAY_TIMEOUT_REFRESH), pdTRUE, 0, timer_hearbeat);
  xTimerStart(xTimerDisplayRefresh, 0);
}

/**
 * @brief main loop of LoRaHome Dongle
 *
 * feed watchdog if messages are regularly forwarded to the broker
 * ensure mqtt connexion to the broker remain active
 * peek messages received, and forward them to the MQTT broker
 *
 */
void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
