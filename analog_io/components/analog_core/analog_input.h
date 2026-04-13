#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include <stdbool.h>
#include <stdint.h>
/*
 * Public Modbus registers expose the product contract:
 * - AI_RAW: 12-bit module-facing raw value
 * - AI_SCALED: 0..10000 mV
 *
 * Additional internal/debug registers may expose board-domain values or
 * native ADS1115 codes for calibration and bring-up.
 */
void AnalogInput_Init(uint8_t ads1115_address);
void AnalogInput_Process(void);
bool AnalogInput_ReadInputRegister(uint16_t address, uint16_t *value);
uint16_t AnalogInput_GetStatus(void);
uint16_t AnalogInput_GetErrorCode(void);

#endif /* ANALOG_INPUT_H */
