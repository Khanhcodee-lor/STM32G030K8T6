#include "analog_output.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

#define MCP4728_ADDRESS_MIN          0x60U
#define MCP4728_ADDRESS_MAX          0x67U
#define MCP4728_GENERAL_CALL_ADDRESS 0x00U
#define MCP4728_GENERAL_CALL_UPDATE  0x08U

extern I2C_HandleTypeDef hi2c2;

static uint8_t s_mcp4728_address;
static uint8_t s_dirty;
static uint32_t s_apply_tick_ms;
static uint32_t s_retry_tick_ms;
static uint16_t s_setpoints[APP_AO_CHANNEL_COUNT];
static uint16_t s_modes[APP_AO_CHANNEL_COUNT];
static uint16_t s_status_register;
static uint16_t s_error_code;

static bool AnalogOutput_IsHoldingRegisterAddress(uint16_t address);
static bool AnalogOutput_FlushOutputs(void);
static void AnalogOutput_MarkDirty(void);

void AnalogOutput_Init(uint8_t mcp4728_address)
{
  (void)memset(s_setpoints, 0, sizeof(s_setpoints));
  (void)memset(s_modes, 0, sizeof(s_modes));

  s_dirty = 0U;
  s_apply_tick_ms = HAL_GetTick();
  s_retry_tick_ms = 0U;
  s_status_register = 0U;
  s_error_code = APP_ERROR_CODE_NONE;

  if ((mcp4728_address < MCP4728_ADDRESS_MIN) || (mcp4728_address > MCP4728_ADDRESS_MAX))
  {
    s_mcp4728_address = 0U;
    s_status_register = APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_AO_DAC_FAULT;
    return;
  }

  s_mcp4728_address = mcp4728_address;
  AnalogOutput_MarkDirty();
}

void AnalogOutput_Process(void)
{
  if ((s_mcp4728_address == 0U) || (s_dirty == 0U))
  {
    return;
  }

  if ((HAL_GetTick() - s_retry_tick_ms) < APP_AO_APPLY_RETRY_DELAY_MS)
  {
    return;
  }

  if ((HAL_GetTick() - s_apply_tick_ms) < APP_AO_APPLY_DELAY_MS)
  {
    return;
  }

  if (AnalogOutput_FlushOutputs())
  {
    s_dirty = 0U;
    s_status_register &= (uint16_t)~APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_NONE;
  }
  else
  {
    s_status_register |= APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_AO_DAC_FAULT;
    s_retry_tick_ms = HAL_GetTick();
  }
}

bool AnalogOutput_ReadHoldingRegister(uint16_t address, uint16_t *value)
{
  if ((value == NULL) || !AnalogOutput_IsHoldingRegisterAddress(address))
  {
    return false;
  }

  if ((address >= APP_AO_HOLDING_REG_SETPOINT_BASE) &&
      (address < (APP_AO_HOLDING_REG_SETPOINT_BASE + APP_AO_CHANNEL_COUNT)))
  {
    *value = s_setpoints[address - APP_AO_HOLDING_REG_SETPOINT_BASE];
    return true;
  }

  *value = s_modes[address - APP_AO_HOLDING_REG_MODE_BASE];
  return true;
}

bool AnalogOutput_WriteHoldingRegister(uint16_t address, uint16_t value)
{
  if (!AnalogOutput_IsHoldingRegisterAddress(address))
  {
    return false;
  }

  if ((address >= APP_AO_HOLDING_REG_SETPOINT_BASE) &&
      (address < (APP_AO_HOLDING_REG_SETPOINT_BASE + APP_AO_CHANNEL_COUNT)))
  {
    if (value > APP_AO_SETPOINT_MAX)
    {
      return false;
    }

    s_setpoints[address - APP_AO_HOLDING_REG_SETPOINT_BASE] = value;
    AnalogOutput_MarkDirty();
    return true;
  }

  if (value > APP_AO_MODE_CURRENT)
  {
    return false;
  }

  s_modes[address - APP_AO_HOLDING_REG_MODE_BASE] = value;
  return true;
}

bool AnalogOutput_WriteHoldingRegisters(uint16_t start_address, const uint16_t *values, uint16_t count)
{
  uint16_t index = 0U;

  if ((values == NULL) || (count == 0U))
  {
    return false;
  }

  for (index = 0U; index < count; ++index)
  {
    if (!AnalogOutput_WriteHoldingRegister((uint16_t)(start_address + index), values[index]))
    {
      return false;
    }
  }

  return true;
}

uint16_t AnalogOutput_GetStatus(void)
{
  return s_status_register;
}

uint16_t AnalogOutput_GetErrorCode(void)
{
  return s_error_code;
}

static bool AnalogOutput_IsHoldingRegisterAddress(uint16_t address)
{
  return (address < APP_AO_HOLDING_REG_COUNT);
}

static bool AnalogOutput_FlushOutputs(void)
{
  uint8_t frame[APP_AO_CHANNEL_COUNT * 2U];
  uint8_t channel = 0U;
  uint8_t general_call_command = MCP4728_GENERAL_CALL_UPDATE;

  for (channel = 0U; channel < APP_AO_CHANNEL_COUNT; ++channel)
  {
    frame[channel * 2U] = (uint8_t)((s_setpoints[channel] >> 8U) & 0x0FU);
    frame[channel * 2U + 1U] = (uint8_t)(s_setpoints[channel] & 0x00FFU);
  }

  if (HAL_I2C_Master_Transmit(
          &hi2c2,
          (uint16_t)(s_mcp4728_address << 1U),
          frame,
          sizeof(frame),
          APP_AO_MCP4728_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }

  return HAL_I2C_Master_Transmit(
             &hi2c2,
             (uint16_t)(MCP4728_GENERAL_CALL_ADDRESS << 1U),
             &general_call_command,
             1U,
             APP_AO_MCP4728_I2C_TIMEOUT_MS) == HAL_OK;
}

static void AnalogOutput_MarkDirty(void)
{
  s_dirty = 1U;
  s_apply_tick_ms = HAL_GetTick();
}
