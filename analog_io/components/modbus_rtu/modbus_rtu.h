#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>

void ModbusRtu_Init(void);
void ModbusRtu_Poll(uint8_t slave_address);
uint16_t ModbusRtu_GetStatusRegister(void);
uint16_t ModbusRtu_GetErrorCode(void);

#endif /* MODBUS_RTU_H */
