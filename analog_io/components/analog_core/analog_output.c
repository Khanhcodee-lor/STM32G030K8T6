#include "analog_output.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

#define MCP4728_ADDRESS_MIN          0x60U
#define MCP4728_ADDRESS_MAX          0x67U
#define MCP4728_GENERAL_CALL_ADDRESS 0x00U
#define MCP4728_GENERAL_CALL_UPDATE  0x08U
#define MCP4728_MULTI_WRITE_COMMAND  0x40U
#define MCP4728_MULTI_WRITE_STRIDE   3U
#define MCP4728_BOOTSTRAP_ADDR_TRIALS APP_I2C_SCAN_READY_TRIALS

extern I2C_HandleTypeDef hi2c2;

static uint8_t s_mcp4728_address;
static uint8_t s_dirty;
static uint32_t s_apply_tick_ms;
static uint32_t s_retry_tick_ms;
static uint16_t s_setpoints[MODBUS_MAP_AO_CHANNEL_COUNT];
static uint16_t s_modes[MODBUS_MAP_AO_CHANNEL_COUNT];
static uint16_t s_status_register;
static uint16_t s_error_code;
static uint8_t s_shadow_state_loaded;

static bool AnalogOutput_IsSetpointRegisterAddress(uint16_t address);
static bool AnalogOutput_IsModeRegisterAddress(uint16_t address);
static bool AnalogOutput_FlushOutputs(void);
static bool AnalogOutput_FlushOutputsToAddress(uint8_t mcp4728_address, uint8_t pd_override_enable, uint8_t pd_override_bits);
static void AnalogOutput_MarkDirty(void);
static void AnalogOutput_LoadSafeDefaults(void);
static bool AnalogOutput_IsValidDeviceAddress(uint8_t mcp4728_address);
static bool AnalogOutput_ProbeDeviceAddress(uint8_t *mcp4728_address);
static uint16_t AnalogOutput_ConvertSetpointToDacCode(uint16_t setpoint, uint16_t mode);
static uint8_t AnalogOutput_GetModeVrefBit(uint16_t mode);
static uint8_t AnalogOutput_GetModePdBits(uint16_t mode);
static uint8_t AnalogOutput_GetModeGainBit(uint16_t mode);

void AnalogOutput_Init(uint8_t mcp4728_address)
{
  if (s_shadow_state_loaded == 0U)
  {
    AnalogOutput_ResetToSafeDefaults();
  }

  s_apply_tick_ms = HAL_GetTick();
  s_retry_tick_ms = 0U;

  if (!AnalogOutput_IsValidDeviceAddress(mcp4728_address))
  {
    s_mcp4728_address = 0U;
    s_status_register |= APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_AO_DAC_FAULT;
    return;
  }

  s_mcp4728_address = mcp4728_address;
  s_status_register &= (uint16_t)~APP_STATUS_AO_FAULT_BIT;
  s_error_code = APP_ERROR_CODE_NONE;
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

void AnalogOutput_ResetToSafeDefaults(void)
{
  AnalogOutput_LoadSafeDefaults();
  s_shadow_state_loaded = 1U;
  s_mcp4728_address = 0U;
  s_dirty = 0U;
  s_apply_tick_ms = HAL_GetTick();
  s_retry_tick_ms = 0U;
  s_status_register = 0U;
  s_error_code = APP_ERROR_CODE_NONE;
}

bool AnalogOutput_ApplySafeStateBlocking(uint32_t timeout_ms)
{
  uint32_t start_tick_ms = HAL_GetTick();
  uint8_t mcp4728_address = s_mcp4728_address;

  if (s_shadow_state_loaded == 0U)
  {
    AnalogOutput_ResetToSafeDefaults();
  }

  if (!AnalogOutput_IsValidDeviceAddress(mcp4728_address) &&
      !AnalogOutput_ProbeDeviceAddress(&mcp4728_address))
  {
    s_mcp4728_address = 0U;
    s_status_register |= APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_AO_DAC_FAULT;
    return false;
  }

  while ((HAL_GetTick() - start_tick_ms) < timeout_ms)
  {
    if (!AnalogOutput_FlushOutputsToAddress(mcp4728_address, 1U, APP_AO_SAFE_BOOT_HOLD_PD_BITS))
    {
      continue;
    }

    if (!AnalogOutput_FlushOutputsToAddress(mcp4728_address, 0U, 0U))
    {
      continue;
    }

    s_mcp4728_address = mcp4728_address;
    s_dirty = 0U;
    s_status_register &= (uint16_t)~APP_STATUS_AO_FAULT_BIT;
    s_error_code = APP_ERROR_CODE_NONE;
    return true;
  }

  s_mcp4728_address = 0U;
  s_status_register |= APP_STATUS_AO_FAULT_BIT;
  s_error_code = APP_ERROR_CODE_AO_DAC_FAULT;
  return false;
}
  
bool AnalogOutput_ReadHoldingRegister(uint16_t address, uint16_t *value)
{
  if ((value == NULL) || !AnalogOutput_IsHoldingRegisterAddress(address))
  {
    return false;
  }

  if (AnalogOutput_IsSetpointRegisterAddress(address))
  {
    *value = s_setpoints[address - MODBUS_MAP_HOLDING_REG_AO_SETPOINT_BASE];
    return true;
  }

  *value = s_modes[address - MODBUS_MAP_HOLDING_REG_AO_MODE_BASE];
  return true;
}

bool AnalogOutput_WriteHoldingRegister(uint16_t address, uint16_t value)
{
  if (!AnalogOutput_IsHoldingRegisterAddress(address))
  {
    return false;
  }

  if (AnalogOutput_IsSetpointRegisterAddress(address))
  {
    if (value > APP_AO_SETPOINT_MAX)
    {
      return false;
    }

    s_setpoints[address - MODBUS_MAP_HOLDING_REG_AO_SETPOINT_BASE] = value;
    AnalogOutput_MarkDirty();
    return true;
  }

  if (!AnalogOutput_IsModeRegisterAddress(address))
  {
    return false;
  }

  if (value > APP_AO_MODE_CURRENT)
  {
    return false;
  }

  s_modes[address - MODBUS_MAP_HOLDING_REG_AO_MODE_BASE] = value;
  AnalogOutput_MarkDirty();
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

bool AnalogOutput_IsHoldingRegisterAddress(uint16_t address)
{
  return AnalogOutput_IsSetpointRegisterAddress(address) ||
         AnalogOutput_IsModeRegisterAddress(address);
}

bool AnalogOutput_IsHoldingRegisterRange(uint16_t start_address, uint16_t count)
{
  uint16_t index = 0U;

  if (count == 0U)
  {
    return false;
  }

  for (index = 0U; index < count; ++index)
  {
    if (!AnalogOutput_IsHoldingRegisterAddress((uint16_t)(start_address + index)))
    {
      return false;
    }
  }

  return true;
}

static bool AnalogOutput_IsSetpointRegisterAddress(uint16_t address)
{
  return (uint16_t)(address - MODBUS_MAP_HOLDING_REG_AO_SETPOINT_BASE) < MODBUS_MAP_AO_CHANNEL_COUNT;
}

static bool AnalogOutput_IsModeRegisterAddress(uint16_t address)
{
  return (uint16_t)(address - MODBUS_MAP_HOLDING_REG_AO_MODE_BASE) < MODBUS_MAP_AO_CHANNEL_COUNT;
}

static bool AnalogOutput_FlushOutputs(void)
{
  return AnalogOutput_FlushOutputsToAddress(s_mcp4728_address, 0U, 0U);
}

static bool AnalogOutput_FlushOutputsToAddress(uint8_t mcp4728_address, uint8_t pd_override_enable, uint8_t pd_override_bits)
{
  uint8_t frame[MODBUS_MAP_AO_CHANNEL_COUNT * MCP4728_MULTI_WRITE_STRIDE];
  uint8_t channel = 0U;
  uint8_t general_call_command = MCP4728_GENERAL_CALL_UPDATE;
  uint8_t *channel_frame = NULL;
  uint8_t vref_bit = 0U;
  uint8_t pd_bits = 0U;
  uint8_t gain_bit = 0U;
  uint16_t dac_code = 0U;

  for (channel = 0U; channel < MODBUS_MAP_AO_CHANNEL_COUNT; ++channel)
  {
    channel_frame = &frame[channel * MCP4728_MULTI_WRITE_STRIDE];
    vref_bit = AnalogOutput_GetModeVrefBit(s_modes[channel]);
    pd_bits = (pd_override_enable != 0U) ? (pd_override_bits & 0x03U) : AnalogOutput_GetModePdBits(s_modes[channel]);
    gain_bit = AnalogOutput_GetModeGainBit(s_modes[channel]);
    dac_code = AnalogOutput_ConvertSetpointToDacCode(s_setpoints[channel], s_modes[channel]);

    channel_frame[0] = (uint8_t)(MCP4728_MULTI_WRITE_COMMAND |
                                 ((channel & 0x03U) << 1U) |
                                 (APP_AO_MCP4728_UDAC_BIT & 0x01U));
    channel_frame[1] = (uint8_t)(((vref_bit & 0x01U) << 7U) |
                                 ((pd_bits & 0x03U) << 5U) |
                                 ((gain_bit & 0x01U) << 4U) |
                                 ((dac_code >> 8U) & 0x0FU));
    channel_frame[2] = (uint8_t)(dac_code & 0x00FFU);
  }

  if (HAL_I2C_Master_Transmit(
          &hi2c2,
          (uint16_t)(mcp4728_address << 1U),
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

static void AnalogOutput_LoadSafeDefaults(void)
{
  s_setpoints[0] = APP_AO_SAFE_DEFAULT_CH1_SETPOINT;
  s_setpoints[1] = APP_AO_SAFE_DEFAULT_CH2_SETPOINT;
  s_setpoints[2] = APP_AO_SAFE_DEFAULT_CH3_SETPOINT;
  s_setpoints[3] = APP_AO_SAFE_DEFAULT_CH4_SETPOINT;
  s_modes[0] = APP_AO_SAFE_DEFAULT_CH1_MODE;
  s_modes[1] = APP_AO_SAFE_DEFAULT_CH2_MODE;
  s_modes[2] = APP_AO_SAFE_DEFAULT_CH3_MODE;
  s_modes[3] = APP_AO_SAFE_DEFAULT_CH4_MODE;
}

static bool AnalogOutput_IsValidDeviceAddress(uint8_t mcp4728_address)
{
  return (mcp4728_address >= MCP4728_ADDRESS_MIN) && (mcp4728_address <= MCP4728_ADDRESS_MAX);
}

static bool AnalogOutput_ProbeDeviceAddress(uint8_t *mcp4728_address)
{
  uint8_t address = 0U;

  if (mcp4728_address == NULL)
  {
    return false;
  }

  for (address = MCP4728_ADDRESS_MIN; address <= MCP4728_ADDRESS_MAX; ++address)
  {
    if (HAL_I2C_IsDeviceReady(
            &hi2c2,
            (uint16_t)(address << 1U),
            MCP4728_BOOTSTRAP_ADDR_TRIALS,
            APP_I2C_SCAN_TIMEOUT_MS) == HAL_OK)
    {
      *mcp4728_address = address;
      return true;
    }
  }

  return false;
}

static uint16_t AnalogOutput_ConvertSetpointToDacCode(uint16_t setpoint, uint16_t mode)
{
  uint32_t dac_code = 0U;
  uint16_t dac_min_code = APP_AO_MODE_VOLTAGE_DAC_MIN_CODE;
  uint16_t dac_max_code = APP_AO_MODE_VOLTAGE_DAC_MAX_CODE;

  if (mode == APP_AO_MODE_CURRENT)
  {
    dac_min_code = APP_AO_MODE_CURRENT_DAC_MIN_CODE;
    dac_max_code = APP_AO_MODE_CURRENT_DAC_MAX_CODE;
  }

  dac_code = (uint32_t)dac_max_code - dac_min_code;
  dac_code = ((uint32_t)setpoint * dac_code + (APP_AO_SETPOINT_MAX / 2U)) / APP_AO_SETPOINT_MAX;
  dac_code += dac_min_code;

  if (dac_code > APP_AO_SETPOINT_MAX)
  {
    dac_code = APP_AO_SETPOINT_MAX;
  }

  return (uint16_t)dac_code;
}

static uint8_t AnalogOutput_GetModeVrefBit(uint16_t mode)
{
  if (mode == APP_AO_MODE_CURRENT)
  {
    return APP_AO_MCP4728_MODE_CURRENT_VREF_BIT & 0x01U;
  }

  return APP_AO_MCP4728_MODE_VOLTAGE_VREF_BIT & 0x01U;
}

static uint8_t AnalogOutput_GetModePdBits(uint16_t mode)
{
  if (mode == APP_AO_MODE_CURRENT)
  {
    return APP_AO_MCP4728_MODE_CURRENT_PD_BITS & 0x03U;
  }

  return APP_AO_MCP4728_MODE_VOLTAGE_PD_BITS & 0x03U;
}

static uint8_t AnalogOutput_GetModeGainBit(uint16_t mode)
{
  if (mode == APP_AO_MODE_CURRENT)
  {
    return APP_AO_MCP4728_MODE_CURRENT_GAIN_BIT & 0x01U;
  }

  return APP_AO_MCP4728_MODE_VOLTAGE_GAIN_BIT & 0x01U;
}
