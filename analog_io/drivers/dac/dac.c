#include "dac.h"

#include "analog_output.h"

void dac_init(uint8_t address)
{
  AnalogOutput_Init(address);
}

void dac_process(void)
{
  AnalogOutput_Process();
}

bool dac_read_holding_register(uint16_t address, uint16_t *value)
{
  return AnalogOutput_ReadHoldingRegister(address, value);
}

bool dac_write_holding_register(uint16_t address, uint16_t value)
{
  return AnalogOutput_WriteHoldingRegister(address, value);
}

bool dac_write_holding_registers(uint16_t start_address, const uint16_t *values, uint16_t count)
{
  return AnalogOutput_WriteHoldingRegisters(start_address, values, count);
}

uint16_t dac_get_status(void)
{
  return AnalogOutput_GetStatus();
}

uint16_t dac_get_error_code(void)
{
  return AnalogOutput_GetErrorCode();
}
