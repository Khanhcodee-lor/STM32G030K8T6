#ifndef ANALOG_OUTPUT_H
#define ANALOG_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

void AnalogOutput_Init(uint8_t mcp4728_address);
void AnalogOutput_Process(void);
bool AnalogOutput_ReadHoldingRegister(uint16_t address, uint16_t *value);
bool AnalogOutput_WriteHoldingRegister(uint16_t address, uint16_t value);
bool AnalogOutput_WriteHoldingRegisters(uint16_t start_address, const uint16_t *values, uint16_t count);
uint16_t AnalogOutput_GetStatus(void);
uint16_t AnalogOutput_GetErrorCode(void);

#endif /* ANALOG_OUTPUT_H */
