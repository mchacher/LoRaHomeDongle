#ifndef SERIAL_API
#define SERIAL_API_

#define DATA_BUFFER_SIZE 128


typedef enum {
  SERIAL_MSG_TYPE_NULL          = 0,
  SERIAL_MSG_TYPE_LOG           = 1,
  SERIAL_MSG_TYPE_SYS           = 2,
  SERIAL_MSG_TYPE_LORA_HOME     = 3
} SERIAL_MSG_TYPE;

typedef enum {
  TYPE_SYS_HEARTBEAT          = 0,
  TYPE_SYS_ECHO               = 1, /** not use **/
  TYPE_SYS_INFO               = 2,
  TYPE_SYS_RESET              = 255
} TYPE_SYS;

typedef struct __attribute__((__packed__)) {
  uint16_t packet_id;
  uint8_t type;
  uint8_t data_length;
} SERIAL_PACKET_HEADER;

typedef struct __attribute__((__packed__)) {
  SERIAL_PACKET_HEADER header;
  uint8_t data[DATA_BUFFER_SIZE];
} SERIAL_PACKET;

void serial_api_send_log_message(char *msg);
void serial_api_send_lora_home_buffer(uint8_t *packet, uint8_t size);
// void serial_api_send_red_mesh_sys_message(RED_MESH_DONGLE_PAYLOAD rmdp, uint8_t length);
// void serial_api_send_red_mesh_tunneling(uint8_t *packet_content);

bool serial_api_get_rx_packet(uint8_t *packet);

#endif
