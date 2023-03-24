/*
 * Module: uart.c
 * ----------------------------
 * UART data link layer
 * use bytestuffing with START, STOP and ESC bytes
 */

#include <Arduino.h>
#include <uart.h>
#include <esp_system.h>

QueueHandle_t rxQueue;

#define WHITE_LED 25

/*
 * Function: get_uart_rx_buffer
 * ----------------------------
 */
bool uart_get_rx_buffer(uint8_t *buffer)
{
  BaseType_t anymsg = xQueueReceive(rxQueue, buffer, 0);
  if (pdTRUE == anymsg)
  {
    return true;
  }
  return false;
}

/*
 * Function: task_uart_rx
 * ----------------------------
 * uart rx task
 */
void task_uart_rx(void *pvParameters)
{

  uint8_t i = 0;
  bool esc_next_byte = false;
  UART_RX_STATE rx_state = RX_IDLE;
  uint8_t rx_buffer[UART_RX_BUFFER_SIZE];

  while (1)
  {
    // // a character has been received
    if (Serial.available())
    {
      uint8_t receivedByte = Serial.read(); // Read the received byte

      switch (rx_state)
      {
      case RX_IDLE:
        if (receivedByte == UART_FLAG_START)
        {
          rx_state = RX_ACTIVE;
          digitalWrite(WHITE_LED, HIGH);
        }
        break;
      case RX_ACTIVE:
        rx_buffer[i++] = receivedByte;
        // manage escaping
        // if escaping, keep character and remove escape_next_byte
        if (esc_next_byte == true)
        {
          esc_next_byte = false;
        }
        // if esc flag, ignore character and wait for next one
        else if (receivedByte == UART_FLAG_ESC)
        {
          esc_next_byte = true;
          i = i - 1;
        }
        // if stop flag, push message in ring buffer, get ready for next rx message
        else if (receivedByte == UART_FLAG_STOP)
        {
          // store message
          xQueueSendToBack(rxQueue, &rx_buffer[0], 0);
          i = 0;
          rx_state = RX_IDLE;
          // esc_next_byte = false;
          digitalWrite(WHITE_LED, LOW);
        }
        break;
      default:
        break;
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/*
 * Function: send_uart_tx_buffer
 * ----------------------------
 * send buffer over uart
 */
bool uart_send_tx_buffer(uint8_t *buffer, uint8_t length)
{
  // if message is too long for tx drop it and return error
  if (UART_TX_BUFFER_SIZE < length)
  {
    return false;
  }
  Serial.write(UART_FLAG_START);
  for (int i = 0; i < length; i++)
  {
    if ((buffer[i] == UART_FLAG_START) || (buffer[i] == UART_FLAG_STOP) || (buffer[i] == UART_FLAG_ESC))
    {
      Serial.write(UART_FLAG_ESC);
    }
    Serial.write(buffer[i]);
  }
  Serial.write(UART_FLAG_STOP);
  Serial.flush();
  return true;
}

/*
 * Function,0x init_uart
 * ----------------------------
 *  initialize uart
 *  use UARTE peripheral with Easy DMA
 */
void uart_init(void)
{
  Serial.begin(115200, SERIAL_8N1);
  pinMode(WHITE_LED, OUTPUT);
  digitalWrite(WHITE_LED, LOW);
  while (!Serial)
    ;
  // Create a queue to hold received messages
  rxQueue = xQueueCreate(UART_RX_FIFO_ITEMS, UART_RX_BUFFER_SIZE * sizeof(uint8_t));
  // Enable RX interrupt for Serial on GPIO pin rxPin (3)
  // attachInterrupt(digitalPinToInterrupt(3), serial_Rx_ISR, RISING);
}