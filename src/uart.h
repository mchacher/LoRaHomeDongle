/**
 * @file uart.h
 * @author mchacher
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef UART_H
#define UART_H

/**
 * @def UART_RX_BUFFER_SIZE
 * @brief The size of the UART receive buffer.
 */
#define UART_RX_BUFFER_SIZE 256
/**
 * @def UART_RX_FIFO_ITEMS
 * @brief The number of items in the UART receive FIFO.
 */
#define UART_RX_FIFO_ITEMS 8
/**
 * @def UART_TX_BUFFER_SIZE
 * @brief The size of the UART transmit buffer.
 */
#define UART_TX_BUFFER_SIZE  256
/**
 * @def UART_TX_FIFO_ITEMS
 * @brief The number of items in the UART transmit FIFO.
 */
#define UART_TX_FIFO_ITEMS 8


/**
 * @brief Flag byte for byte stuffing
 * UART_FLAG_START byte that denote the beginning of the frame
 * 
 */
static const uint8_t UART_FLAG_START  = 0x12;

/**
 * @brief Flag byte for byte stuffing
 * UART_FLAG_STOP and an end flag byte will denote its end.
 * 
 */
static const uint8_t UART_FLAG_STOP   = 0x13;

/**
 * @brief Flag byte for byte stuffing
 * UART_FLAG_ESC special escape byte, insert whenever a flag (start, end or esc) byte appears in the data
 */
static const uint8_t UART_FLAG_ESC    = 0x14;

/**
 * @brief UART RX State (actually simply 2) to well manage bytes stuffing decoding
 * 
 */
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
