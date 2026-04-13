#ifndef RS485_H
#define RS485_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void rs485_init(uint32_t baudrate);
void rs485_send_bytes(const uint8_t *data, size_t length);
bool rs485_read_byte(uint8_t *byte);

#endif /* RS485_H */
