#include "rs485.h"

#include "rs485_ll.h"

void rs485_init(uint32_t baudrate)
{
  Rs485Ll_Init(baudrate);
}

void rs485_send_bytes(const uint8_t *data, size_t length)
{
  Rs485Ll_Send(data, length);
}

bool rs485_read_byte(uint8_t *byte)
{
  return Rs485Ll_ReadByte(byte);
}
