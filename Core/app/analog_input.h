#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include <stdbool.h>
#include <stdint.h>
/*
raw là giá trị thô
scaled là giá trị đã quy đổi về mV or V
 */
void AnalogInput_Init(uint8_t ads1115_address);
void AnalogInput_Process(void);
bool AnalogInput_ReadInputRegister(uint16_t address, uint16_t *value);
uint16_t AnalogInput_GetStatus(void);
uint16_t AnalogInput_GetErrorCode(void);

#endif /* ANALOG_INPUT_H */
