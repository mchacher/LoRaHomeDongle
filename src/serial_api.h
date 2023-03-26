#ifndef SERIAL_API_H
#define SERIAL_API_H

#define DATA_BUFFER_SIZE 128

typedef enum
{
  SERIAL_MSG_TYPE_NULL = 0,
  SERIAL_MSG_TYPE_LOG = 1,
  SERIAL_MSG_TYPE_SYS = 2,
  SERIAL_MSG_TYPE_LORA_HOME = 3
} SERIAL_MSG_TYPE;

typedef struct __attribute__((__packed__))
{
  uint16_t packet_id;
  uint8_t type;
  uint8_t data_length;
} SERIAL_PACKET_HEADER;

typedef struct __attribute__((__packed__))
{
  SERIAL_PACKET_HEADER header;
  uint8_t data[DATA_BUFFER_SIZE];
} SERIAL_PACKET;

typedef enum
{
  TYPE_SYS_HEARTBEAT = 1,
  TYPE_SYS_ECHO = 2,
  TYPE_SYS_INFO = 3,
  TYPE_SYS_RESET = 254
} TYPE_SYS;

typedef struct __attribute__((__packed__))
{
  uint8_t sys_type;
  uint8_t payload[DATA_BUFFER_SIZE];
} DONGLE_SYS_PACKET;

typedef struct __attribute__((__packed__))
{
  uint32_t rx_counter;
  uint32_t tx_counter;
  uint32_t err_counter;
} DONGLE_HEARTBEAT_PACKET;

typedef struct __attribute__((__packed__))
{
  uint32_t uid;
  uint8_t version_major;
  uint8_t version_minor;
  uint8_t version_patch;
} DONGLE_VERSION_PACKET;

void serial_api_send_log_message(char *msg);
void serial_api_send_sys_packet(uint8_t *packet, uint8_t size);
void serial_api_send_lora_home_packet(uint8_t *packet, uint8_t size);
bool serial_api_get_rx_packet(uint8_t *packet);

#endif
