#ifndef RS485_LL_H
#define RS485_LL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void Rs485Ll_Init(uint32_t baudrate);
void Rs485Ll_Send(const uint8_t *data, size_t length);
bool Rs485Ll_ReadByte(uint8_t *byte);
void Rs485Ll_IrqHandler(void);

#endif /* RS485_LL_H */
