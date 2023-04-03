#ifndef UART_H
#define UART_H

#define UART_RX_BUFFER_SIZE 256
#define UART_RX_FIFO_ITEMS 8
#define UART_TX_BUFFER_SIZE  256
#define UART_TX_FIFO_ITEMS 8

// Flag bytes for byte stuffing
// basic serial communication protocol with
// - UART_FLAG_START byte that denote the beginning of the frame,
// - UART_FLAG_STOP and an end flag byte will denote its end.
// - UART_FLAG_ESC special escape byte, insert whenever a flag (start, end or esc) byte appears in the data
static const uint8_t UART_FLAG_START  = 0x12;
static const uint8_t UART_FLAG_STOP   = 0x13;
static const uint8_t UART_FLAG_ESC    = 0x14;

// UART RX State (actually simply 2) to well manage by stuffing decoding
typedef enum 
{
  RX_IDLE,
  RX_ACTIVE,
} UART_RX_STATE;


void uart_init();
bool uart_get_rx_buffer(uint8_t *buffer);
bool uart_put_tx_buffer(uint8_t *buffer, uint8_t length);
void task_uart_rx(void *pvParameters);
void task_uart_tx(void *pvParameters);

#endif
