#include <Arduino.h>
#include <serial_api.h>
#include <uart.h>

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
  uart_send_tx_buffer((uint8_t *)&sp, sph.data_length + sizeof(sph));
}

/*
 * Function: serial_api_send_lora_buffer
 * ----------------------------
 *  send a log message
 *  - type of the message: MSG_TYPE_LOG
 *  - * message: the message to send
 */
void serial_api_send_lora_home_buffer(uint8_t *packet, uint8_t size)
{
  SERIAL_PACKET_HEADER sph = {0};
  sph.packet_id = _packet_id++;
  sph.type = SERIAL_MSG_TYPE_LORA_HOME;
  sph.data_length = size;
  SERIAL_PACKET sp = {0};
  sp.header = sph;
  memcpy(sp.data, packet, size);
  uart_send_tx_buffer((uint8_t *)&sp, sph.data_length + sizeof(sph));
}

bool serial_api_get_rx_packet(uint8_t *packet)
{
  return uart_get_rx_buffer(packet);
}



// /*
//  * Function: serial_api_send_red_mesh_sys_message
//  * ----------------------------
//  *  send a system message
//  *  - type of the message: MSG_TYPE_DONGLE_SYS
//  *  - rmdp : the red mesh dongle payload
//  *  - length: length of the message (number of bytes)
//  */
// void serial_api_send_red_mesh_sys_message(RED_MESH_DONGLE_PAYLOAD rmdp, uint8_t length)
// {
//   SERIAL_PACKET_HEADER sph = {0};
//   sph.packet_id = _packet_id++;
//   sph.type = SERIAL_MSG_TYPE_RED_MESH_DONGLE_SYS;
//   sph.data_length = length;
//   SERIAL_PACKET sp = {0};
//   sp.header = sph;
//   memcpy(sp.data, &rmdp, length);
//   length += sizeof(sph);
//   send_to_uart(&sp, length);
// }

// /*
//  * Function: serial_api_send_red_mesh_tunneling
//  * ----------------------------
//  *  send a red mesh tunneling message
//  *  - type of the message: SERIAL_MSG_TYPE_RED_MESH_TUNNELING
//  *  - packet_content : the red mesh packet content
//  */
// void serial_api_send_red_mesh_tunneling(uint8_t *packet_content)
// {
//   SERIAL_PACKET_HEADER sph = {0};
//   sph.packet_id = _packet_id++;
//   sph.type = SERIAL_MSG_TYPE_RED_MESH_TUNNELING;
//   sph.data_length = sizeof(MESH_PACKET_CONTENT);
//   SERIAL_PACKET sp = {0};
//   sp.header = sph;
//   memcpy(sp.data, packet_content, sizeof(MESH_PACKET_CONTENT));
//   send_to_uart(&sp, sph.data_length + sizeof(sph));
// }

// /*
//  * Function: serial_api_get_rx_packet
//  * ----------------------------
//  *  get serial message in RX buffer and feed corresponding serial packet
//  *  check if any error
//  *  *packet : serial packet to return
//  */
// ret_code_t serial_api_get_rx_packet(SERIAL_PACKET *packet)
// {
//   uint8_t len_in;
//   uart_get_rx_buffer(_rx_buffer, &len_in);
//   if (len_in > 0) {
//     if (len_in < sizeof(SERIAL_PACKET_HEADER) + 1){
//       return NRF_ERROR_DATA_SIZE;
//     }
//     // NRF_LOG_INFO("serial_api_get_rx_packet");
//     packet->header.packet_id = (_rx_buffer[0] << 8) | _rx_buffer[1];
//     // NRF_LOG_INFO("--- packet id: %04x", packet->header.packet_id);
//     packet->header.type = _rx_buffer[2];
//     // NRF_LOG_INFO("--- type: %02x", packet->header.type);
//     packet->header.data_length = _rx_buffer[3];
//     if (len_in != (sizeof(SERIAL_PACKET_HEADER) + packet->header.data_length)){
//       return NRF_ERROR_DATA_SIZE;
//     }
//     memcpy(packet->data, &(_rx_buffer[4]), packet->header.data_length);

//     // for (int i = 0; i < packet->header.data_length; i++){
//        // NRF_LOG_INFO("--- %02x", packet->data[i]);
//     //}
//   }
//   return NRF_SUCCESS;
// }