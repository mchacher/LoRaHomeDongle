/*
 * Module: uart.c
 * ----------------------------
 * UART data link layer
 * use bytestuffing with START, STOP and ESC bytes
 */

#include <Arduino.h>
#include <uart.h>
#include <esp_system.h>

QueueHandle_t rx_uart_queue;
QueueHandle_t tx_uart_queue;

#define WHITE_LED 25

/*
 * Function: uart_get_rx_buffer
 * return rx buffer is any available in the rx queue
 * ----------------------------
 */
bool uart_get_rx_buffer(uint8_t *buffer)
{
  BaseType_t anymsg = xQueueReceive(rx_uart_queue, buffer, 0);
  if (pdTRUE == anymsg)
  {
    return true;
  }
  return false;
}

/*
 * Function: uart_put_tx_buffer
 * ----------------------------
 * push a buffer to tx queue. Add byte stuffing prior to pushing it to the queue.
 */
bool uart_put_tx_buffer(uint8_t *buffer, uint8_t length)
{
  // if message is too long for tx drop it and return error
  // max size is UART_TX_BUFFER_SIZE -2, since at least START and STOP bytes will be added
  if ((UART_TX_BUFFER_SIZE -2) < length)
  {
    return false;
  }
  uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
  uint8_t index = 1;
  tx_buffer[index++] = UART_FLAG_START;
  for (int i = 0; i < length; i++)
  {
    if ((buffer[i] == UART_FLAG_START) || (buffer[i] == UART_FLAG_STOP) || (buffer[i] == UART_FLAG_ESC))
    {
      tx_buffer[index++] = UART_FLAG_ESC;
    }
    tx_buffer[index++] = buffer[i];
    // return if index > UART_TX_BUFFER_SIZE
    if (index > (UART_TX_BUFFER_SIZE -1))
    {
      return false;
    }
  }
  tx_buffer[index++] = UART_FLAG_STOP;
  tx_buffer[0] = index;
  xQueueSendToBack(tx_uart_queue, tx_buffer, 0);
  return true;
}

/*
 * Function: task_uart_rx
 * ----------------------------
 * uart rx task
 * decode incoming message and push it to rx queue
 */
void task_uart_rx(void *pvParameters)
{

  uint8_t i = 0;
  bool esc_next_byte = false;
  UART_RX_STATE rx_state = RX_IDLE;
  uint8_t rx_buffer[UART_RX_BUFFER_SIZE];

  while (1)
  {
    // a character has been received
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
          xQueueSendToBack(rx_uart_queue, &rx_buffer[0], 0);
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
 * Function task_uart_tx
 * ----------------------------
 *  task managed by FreeRTOS
 *  check whether there is a buffer to be sent out in the tx queue
 *  send it if any
 */
void task_uart_tx(void *pvParameters)
{
  uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
  while (1)
  {
    BaseType_t anymsg = xQueueReceive(tx_uart_queue, tx_buffer, 0);
    if (pdTRUE == anymsg)
    {
      for (int i = 1; i < tx_buffer[0]; i++)
      Serial.write(tx_buffer[i]);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


/*
 * Function init_uart
 * ----------------------------
 *  initialize uart
 *  create rx and tx queue and initialized WHITE_LED as activity led
 */
void uart_init(void)
{
  Serial.begin(115200, SERIAL_8N1);
  pinMode(WHITE_LED, OUTPUT);
  digitalWrite(WHITE_LED, LOW);
  while (!Serial)
    ;
  // Create a queue to hold messages
  rx_uart_queue = xQueueCreate(UART_RX_FIFO_ITEMS, UART_RX_BUFFER_SIZE * sizeof(uint8_t));
  tx_uart_queue = xQueueCreate(UART_TX_FIFO_ITEMS, UART_TX_BUFFER_SIZE * sizeof(uint8_t));
}
