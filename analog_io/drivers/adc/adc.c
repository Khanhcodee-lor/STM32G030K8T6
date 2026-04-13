#include "adc.h"

#include "analog_input.h"

void adc_init(uint8_t address)
{
  AnalogInput_Init(address);
}

void adc_process(void)
{
  AnalogInput_Process();
}

bool adc_read_input_register(uint16_t address, uint16_t *value)
{
  return AnalogInput_ReadInputRegister(address, value);
}

uint16_t adc_get_status(void)
{
  return AnalogInput_GetStatus();
}

uint16_t adc_get_error_code(void)
{
  return AnalogInput_GetErrorCode();
}
