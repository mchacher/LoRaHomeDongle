#include <Arduino.h>
#include "serial_api.h"
#include "uart.h"

/*
 * Module: serial_api.c
 * ----------------------------
 * serial applicative layer on top of uart data link
 * provide generic SERIAL_PACKET to exchange data with host
 * provide 4 types of SERIAL_PACKET to communication with host : 
 * - log: simply send a text information to the dongle (not used)
 * - echo: for test purpose, dongle can send an echo message, that is echo to the host
 * - system: different types of system messages (heartbeat, node info (meaning information about connected equipment, more to come ...)
 */


static uint16_t _packet_id = 0x0000;
QueueHandle_t sys_packet_queue;

/*
 * Function: serial_api_send_log_message
 * ----------------------------
 *  send a log message
 *  - type of the message: MSG_TYPE_LOG
 *  - * message: the message to send
 */
void serial_api_send_log_message(char *message)
{
  SERIAL_PACKET_HEADER sph = {0};
  sph.packet_id = _packet_id++;
  sph.type = SERIAL_MSG_TYPE_LOG;
  sph.data_length = strlen(message);
  SERIAL_PACKET sp = {0};
  sp.header = sph;
  memcpy(sp.data, message, strlen(message));
  uart_put_tx_buffer((uint8_t *)&sp, sph.data_length + sizeof(sph));
}

/*
 * Function: serial_api_send_lora_packet
 * ----------------------------
 *  send a log message
 *  - type of the message: SERIAL_MSG_TYPE_LORA_HOME
 */
void serial_api_send_lora_home_packet(uint8_t *packet, uint8_t size)
{
  SERIAL_PACKET_HEADER sph = {0};
  sph.packet_id = _packet_id++;
  sph.type = SERIAL_MSG_TYPE_LORA_HOME;
  sph.data_length = size;
  SERIAL_PACKET sp = {0};
  sp.header = sph;
  memcpy(sp.data, packet, size);
  uart_put_tx_buffer((uint8_t *)&sp, sph.data_length + sizeof(sph));
}


void serial_api_send_sys_packet(uint8_t *packet, uint8_t size)
{
  SERIAL_PACKET_HEADER sph = {0};
  sph.packet_id = _packet_id++;
  sph.type = SERIAL_MSG_TYPE_SYS;
  sph.data_length = size;
  SERIAL_PACKET sp = {0};
  sp.header = sph;
  memcpy(sp.data, packet, size);
  uart_put_tx_buffer((uint8_t *)&sp, sph.data_length + sizeof(sph));
}

bool serial_api_get_system_packet(uint8_t *packet)
{
  BaseType_t anymsg = xQueueReceive(sys_packet_queue, packet, 0);
  if (pdTRUE == anymsg)
  {
    return true;
  }
  return false;
}

bool serial_api_get_lora_home_packet(uint8_t *packet)
{
  while (uart_get_rx_buffer(packet))
  {
    SERIAL_PACKET *sp = (SERIAL_PACKET*)packet;
    if (sp->header.type == SERIAL_MSG_TYPE_LORA_HOME)
    {
      return true;
    }
    // else put the packet in the system queue
    else if(sp->header.type == SERIAL_MSG_TYPE_SYS)
    {
      xQueueSendToBack(sys_packet_queue, packet, 0);
    }
  }
  return false;
}

void serial_api_init(void)
{
  // Create a queue to hold messages
  sys_packet_queue = xQueueCreate(UART_RX_FIFO_ITEMS, UART_RX_BUFFER_SIZE * sizeof(uint8_t));
}