#include <Arduino.h>
#include <esp_system.h>
#include <LoRaHomeGateway.h>
#include <Display.h>
#include <uart.h>
#include <serial_api.h>

// #define DEBUG_ESP_PORT Serial
#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

// uncomment to activate the watchdog
//#define WATCHDOG
// watchdog management
#ifdef WATCHDOG
// time in ms to trigger the watchdog
const int wdtTimeout = 60000;
hw_timer_t *timer = NULL;
#endif

// timeout for refreshing display
#define DISPLAY_TIMEOUT_REFRESH 1000 // 1s

// Display
Display display(loraHomeGateway);

// Tasks and Timers
TaskHandle_t taskHandle = NULL;
TimerHandle_t xTimerDisplayRefresh = NULL;

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

void task_lora_home_sender(void *pvParameters)
{
  SERIAL_PACKET *serial_packet; // Buffer to hold received messages
  LORA_HOME_PACKET *lora_packet;
  uint8_t rx_buffer[256];
  uint8_t packet[LH_FRAME_MAX_SIZE];
  while (1)
  {
    if (uart_get_rx_buffer(rx_buffer))
    {
      serial_packet = (SERIAL_PACKET *)rx_buffer;
      if (serial_packet->header.type == SERIAL_MSG_TYPE_LORA_HOME)
      {
        lora_packet = (LORA_HOME_PACKET *)serial_packet->data;
        // loraHomeGateway.txCounter++;
        loraHomeGateway.sendPacket((uint8_t *)lora_packet);
        // char buffer[256] = "\0";
        // for (int i = 0; i < serial_packet->header.data_length + sizeof(SERIAL_PACKET_HEADER); i++)
        // {
        //   sprintf(buffer + strlen(buffer), "%02x:", rx_buffer[i]);
        // }
        // serial_api_send_log_message(buffer);
      }
      //
    }
    if (loraHomeGateway.popLoRaHomePayload(packet))
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
 * pop lora home packet if any and forward it over the uart
 */
// void task_lora_home_forwarder(void *pvParameters)
// {
//   uint8_t lora_packet[LH_FRAME_MAX_SIZE];
//   while (1)
//   {
//     if (loraHomeGateway.popLoRaHomePayload(lora_packet))
//     {
//       LORA_HOME_PACKET *lhp = (LORA_HOME_PACKET *)lora_packet;
//       uint8_t size = sizeof(LORA_HOME_PACKET_HEADER) + lhp->header.payloadSize; // + LH_FRAME_FOOTER_SIZE;
//       serial_api_send_lora_home_buffer(lora_packet, size);
//     }
//     vTaskDelay(10 / portTICK_PERIOD_MS);
//   }
// }

/**
 * @brief refresh display every DISPLAY_TIMEOUT_REFRESH
 */
void timer_display_refresh(TimerHandle_t xTimer)
{
  display.refresh();
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
  // Display initialization
  display.init();
  // UART initialization
  display.showUsbStatus(false);
  display.showLoRaStatus(false);
  uart_init();
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
  loraHomeGateway.setup();
  DEBUG_MSG("--- LoRa Init OK!\n");
  display.showLoRaStatus(true);
  DEBUG_MSG("Main Loop starting, run on Core %i\n\n", xPortGetCoreID());
  xTaskCreate(task_lora_home_sender, "task_lora_home_sender", 2048, NULL, 1, &taskHandle);
  xTaskCreate(task_uart_rx, "task_uart_rx", 2048, NULL, 1, NULL);
  // xTaskCreate(task_lora_home_forwarder, "task_lora_home_forwarder", 2048, NULL, 1, NULL);
  xTimerDisplayRefresh = xTimerCreate("Display Reflesh", pdMS_TO_TICKS(DISPLAY_TIMEOUT_REFRESH), pdTRUE, 0, timer_display_refresh);
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
  static unsigned long time;
  time = millis();

#ifdef WATCHDOG
  // reset timer (feed watchdog)
  if ((time - loraHomeGateway.lastMsgProcessedTimeStamp) < wdtTimeout)
  {
    timerWrite(timer, 0);
  }
#endif
  vTaskDelay(pdMS_TO_TICKS(1000));

}
