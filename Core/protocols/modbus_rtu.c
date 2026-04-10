#include "modbus_rtu.h"

#include "analog_input.h"
#include "analog_output.h"
#include "app_config.h"
#include "debug_log.h"
#include "device_config.h"
#include "main.h"
#include "rs485_ll.h"
#include <string.h>

#define MODBUS_RTU_MAX_FRAME_SIZE          64U
#define MODBUS_FUNC_READ_HOLDING_REGISTERS 0x03U
#define MODBUS_FUNC_READ_INPUT_REGISTERS   0x04U
#define MODBUS_FUNC_WRITE_SINGLE_REGISTER  0x06U
#define MODBUS_FUNC_WRITE_MULTIPLE_REGS    0x10U
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION  0x01U
#define MODBUS_EXCEPTION_ILLEGAL_ADDRESS   0x02U
#define MODBUS_EXCEPTION_ILLEGAL_VALUE     0x03U
#define MODBUS_EXCEPTION_DEVICE_FAILURE    0x04U
#define MODBUS_RTU_MAX_READ_REGISTERS      ((MODBUS_RTU_MAX_FRAME_SIZE - 5U) / 2U)
#define MODBUS_RTU_MAX_WRITE_REGISTERS     APP_AO_HOLDING_REG_COUNT

static uint8_t s_rx_buffer[MODBUS_RTU_MAX_FRAME_SIZE];
static uint8_t s_rx_length;
static uint32_t s_last_rx_tick_ms;
static uint8_t s_is_receiving;
static uint8_t s_pending_address;
static uint8_t s_pending_address_valid;
static uint32_t s_pending_apply_tick_ms;

static void ModbusRtu_ResetRxState(void);
static void ModbusRtu_ApplyPendingAddress(void);
static void ModbusRtu_ProcessFrame(uint8_t slave_address);
static bool ModbusRtu_ReadHoldingRegister(uint16_t address, uint16_t *value);
static bool ModbusRtu_ReadInputRegister(uint16_t address, uint16_t *value);
static uint8_t ModbusRtu_WriteSingleHoldingRegister(uint16_t address, uint16_t value);
static uint8_t ModbusRtu_WriteMultipleHoldingRegisters(uint16_t start_address, uint16_t count, const uint8_t *data);
static uint8_t ModbusRtu_ValidateHoldingRegisterWrite(uint16_t address, uint16_t value);
static uint16_t ModbusRtu_GetStatusRegister(void);
static uint16_t ModbusRtu_GetErrorCode(void);
static uint16_t ModbusRtu_Crc16(const uint8_t *data, uint8_t length);
static uint16_t ModbusRtu_ReadU16(const uint8_t *data);
static uint16_t ModbusRtu_ReadLeU16(const uint8_t *data);
static void ModbusRtu_WriteU16(uint8_t *data, uint16_t value);
static void ModbusRtu_SendResponse(const uint8_t *data, uint8_t length);
static void ModbusRtu_SendException(uint8_t slave_address, uint8_t function, uint8_t exception_code);
static void ModbusRtu_DebugPrintFrame(const char *prefix, const uint8_t *data, uint8_t length);

void ModbusRtu_Init(void)
{
  ModbusRtu_ResetRxState();
  s_pending_address = 0U;
  s_pending_address_valid = 0U;
  s_pending_apply_tick_ms = 0U;
}

void ModbusRtu_Poll(uint8_t slave_address)
{
  uint8_t byte = 0U;

  while (Rs485Ll_ReadByte(&byte))
  {
    if (s_rx_length < MODBUS_RTU_MAX_FRAME_SIZE)
    {
      s_rx_buffer[s_rx_length++] = byte;
      s_last_rx_tick_ms = HAL_GetTick();
      s_is_receiving = 1U;
    }
    else
    {
      DEBUG_LOG("modbus drop: frame too long\r\n");
      ModbusRtu_ResetRxState();
      return;
    }
  }

  if ((s_is_receiving != 0U) &&
      ((HAL_GetTick() - s_last_rx_tick_ms) >= APP_MODBUS_FRAME_TIMEOUT_MS))
  {
    ModbusRtu_ProcessFrame(slave_address);
    ModbusRtu_ResetRxState();
  }

  ModbusRtu_ApplyPendingAddress();
}

static void ModbusRtu_ResetRxState(void)
{
  s_rx_length = 0U;
  s_last_rx_tick_ms = 0U;
  s_is_receiving = 0U;
}

static void ModbusRtu_ApplyPendingAddress(void)
{
  if ((s_pending_address_valid == 0U) ||
      ((HAL_GetTick() - s_pending_apply_tick_ms) < APP_MODBUS_ADDRESS_APPLY_DELAY_MS))
  {
    return;
  }

  if (!DeviceConfig_SetModbusAddress(s_pending_address))
  {
    DEBUG_LOG("modbus addr set failed: %u\r\n", (unsigned int)s_pending_address);
    s_pending_address_valid = 0U;
    return;
  }

  if (!DeviceConfig_Save())
  {
    DEBUG_LOG("modbus addr save failed: %u\r\n", (unsigned int)s_pending_address);
    s_pending_address_valid = 0U;
    return;
  }

  DEBUG_LOG("modbus addr saved: %u\r\n", (unsigned int)DeviceConfig_GetModbusAddress());
  s_pending_address_valid = 0U;
}

static void ModbusRtu_ProcessFrame(uint8_t slave_address)
{
  uint16_t frame_crc = 0U;
  uint16_t computed_crc = 0U;
  uint8_t function = 0U;
  uint16_t start_address = 0U;
  uint16_t register_count = 0U;

  if (s_rx_length < 4U)
  {
    DEBUG_LOG("modbus drop: short frame\r\n");
    return;
  }

  ModbusRtu_DebugPrintFrame("modbus frame", s_rx_buffer, s_rx_length);

  frame_crc = ModbusRtu_ReadLeU16(&s_rx_buffer[s_rx_length - 2U]);
  computed_crc = ModbusRtu_Crc16(s_rx_buffer, (uint8_t)(s_rx_length - 2U));

  if (frame_crc != computed_crc)
  {
    DEBUG_LOG("modbus drop: crc error rx=0x%04X calc=0x%04X\r\n",
              frame_crc,
              computed_crc);
    return;
  }

  if (s_rx_buffer[0] != slave_address)
  {
    DEBUG_LOG("modbus ignore: slave=%u expected=%u\r\n",
              (unsigned int)s_rx_buffer[0],
              (unsigned int)slave_address);
    return;
  }

  function = s_rx_buffer[1];

  switch (function)
  {
    case MODBUS_FUNC_READ_HOLDING_REGISTERS:
      if (s_rx_length != 8U)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      start_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      register_count = ModbusRtu_ReadU16(&s_rx_buffer[4]);

      if ((register_count == 0U) || (register_count > MODBUS_RTU_MAX_READ_REGISTERS))
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      {
        uint8_t response[MODBUS_RTU_MAX_FRAME_SIZE];
        uint16_t index = 0U;
        uint16_t register_value = 0U;
        uint8_t byte_count = (uint8_t)(register_count * 2U);

        response[0] = slave_address;
        response[1] = function;
        response[2] = byte_count;

        for (index = 0U; index < register_count; ++index)
        {
          if (!ModbusRtu_ReadHoldingRegister((uint16_t)(start_address + index), &register_value))
          {
            ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_ADDRESS);
            return;
          }

          ModbusRtu_WriteU16(&response[3U + (index * 2U)], register_value);
        }

        ModbusRtu_SendResponse(response, (uint8_t)(3U + byte_count));
      }
      break;

    case MODBUS_FUNC_READ_INPUT_REGISTERS:
      if (s_rx_length != 8U)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      start_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      register_count = ModbusRtu_ReadU16(&s_rx_buffer[4]);

      if ((register_count == 0U) || (register_count > MODBUS_RTU_MAX_READ_REGISTERS))
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      {
        uint8_t response[MODBUS_RTU_MAX_FRAME_SIZE];
        uint16_t index = 0U;
        uint16_t register_value = 0U;
        uint8_t byte_count = (uint8_t)(register_count * 2U);

        response[0] = slave_address;
        response[1] = function;
        response[2] = byte_count;

        for (index = 0U; index < register_count; ++index)
        {
          if (!ModbusRtu_ReadInputRegister((uint16_t)(start_address + index), &register_value))
          {
            ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_ADDRESS);
            return;
          }

          ModbusRtu_WriteU16(&response[3U + (index * 2U)], register_value);
        }

        ModbusRtu_SendResponse(response, (uint8_t)(3U + byte_count));
      }
      break;

    case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
      if (s_rx_length != 8U)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      start_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      {
        uint8_t exception_code = ModbusRtu_WriteSingleHoldingRegister(
            start_address,
            ModbusRtu_ReadU16(&s_rx_buffer[4]));

        if (exception_code != 0U)
        {
          ModbusRtu_SendException(slave_address, function, exception_code);
          return;
        }
      }

      ModbusRtu_SendResponse(s_rx_buffer, 6U);
      break;

    case MODBUS_FUNC_WRITE_MULTIPLE_REGS:
      if (s_rx_length < 11U)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      start_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      register_count = ModbusRtu_ReadU16(&s_rx_buffer[4]);

      if ((register_count == 0U) || (register_count > MODBUS_RTU_MAX_WRITE_REGISTERS))
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      if ((s_rx_buffer[6] != (uint8_t)(register_count * 2U)) ||
          (s_rx_length != (uint8_t)(9U + s_rx_buffer[6])))
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      {
        uint8_t exception_code = ModbusRtu_WriteMultipleHoldingRegisters(
            start_address,
            register_count,
            &s_rx_buffer[7]);

        if (exception_code != 0U)
        {
          ModbusRtu_SendException(slave_address, function, exception_code);
          return;
        }
      }

      ModbusRtu_SendResponse(s_rx_buffer, 6U);
      break;

    default:
      ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
      break;
  }
}

static bool ModbusRtu_ReadHoldingRegister(uint16_t address, uint16_t *value)
{
  if (value == NULL)
  {
    return false;
  }

  if (AnalogOutput_ReadHoldingRegister(address, value))
  {
    return true;
  }

  switch (address)
  {
    case APP_DEVICE_REG_DEVICE_ID:
      *value = DeviceConfig_GetModbusAddress();
      return true;

    case APP_DEVICE_REG_FW_VERSION:
      *value = APP_FW_VERSION;
      return true;

    case APP_DEVICE_REG_STATUS:
      *value = ModbusRtu_GetStatusRegister();
      return true;

    case APP_DEVICE_REG_ERROR_CODE:
      *value = ModbusRtu_GetErrorCode();
      return true;

    default:
      return false;
  }
}

static bool ModbusRtu_ReadInputRegister(uint16_t address, uint16_t *value)
{
  return AnalogInput_ReadInputRegister(address, value);
}

static uint8_t ModbusRtu_WriteSingleHoldingRegister(uint16_t address, uint16_t value)
{
  if (address < APP_AO_HOLDING_REG_COUNT)
  {
    return AnalogOutput_WriteHoldingRegister(address, value) ? 0U : MODBUS_EXCEPTION_ILLEGAL_VALUE;
  }

  if (address == APP_DEVICE_REG_DEVICE_ID)
  {
    if ((value < APP_MODBUS_ADDRESS_MIN) || (value > APP_MODBUS_ADDRESS_MAX))
    {
      return MODBUS_EXCEPTION_ILLEGAL_VALUE;
    }

    s_pending_address = (uint8_t)value;
    s_pending_address_valid = 1U;
    s_pending_apply_tick_ms = HAL_GetTick();
    return 0U;
  }

  return MODBUS_EXCEPTION_ILLEGAL_ADDRESS;
}

static uint8_t ModbusRtu_WriteMultipleHoldingRegisters(uint16_t start_address, uint16_t count, const uint8_t *data)
{
  uint16_t index = 0U;
  uint16_t value = 0U;
  uint16_t values[APP_AO_HOLDING_REG_COUNT];

  if ((data == NULL) ||
      (start_address >= APP_AO_HOLDING_REG_COUNT) ||
      ((start_address + count) > APP_AO_HOLDING_REG_COUNT))
  {
    return MODBUS_EXCEPTION_ILLEGAL_ADDRESS;
  }

  for (index = 0U; index < count; ++index)
  {
    value = ModbusRtu_ReadU16(&data[index * 2U]);
    if (ModbusRtu_ValidateHoldingRegisterWrite((uint16_t)(start_address + index), value) != 0U)
    {
      return MODBUS_EXCEPTION_ILLEGAL_VALUE;
    }

    values[index] = value;
  }

  return AnalogOutput_WriteHoldingRegisters(start_address, values, count) ? 0U : MODBUS_EXCEPTION_DEVICE_FAILURE;
}

static uint8_t ModbusRtu_ValidateHoldingRegisterWrite(uint16_t address, uint16_t value)
{
  if ((address >= APP_AO_HOLDING_REG_SETPOINT_BASE) &&
      (address < (APP_AO_HOLDING_REG_SETPOINT_BASE + APP_AO_CHANNEL_COUNT)))
  {
    return (value <= APP_AO_SETPOINT_MAX) ? 0U : MODBUS_EXCEPTION_ILLEGAL_VALUE;
  }

  if ((address >= APP_AO_HOLDING_REG_MODE_BASE) &&
      (address < (APP_AO_HOLDING_REG_MODE_BASE + APP_AO_CHANNEL_COUNT)))
  {
    return (value <= APP_AO_MODE_CURRENT) ? 0U : MODBUS_EXCEPTION_ILLEGAL_VALUE;
  }

  return MODBUS_EXCEPTION_ILLEGAL_ADDRESS;
}

static uint16_t ModbusRtu_GetStatusRegister(void)
{
  return (uint16_t)(AnalogInput_GetStatus() | AnalogOutput_GetStatus());
}

static uint16_t ModbusRtu_GetErrorCode(void)
{
  uint16_t ao_error = AnalogOutput_GetErrorCode();

  if (ao_error != APP_ERROR_CODE_NONE)
  {
    return ao_error;
  }

  return AnalogInput_GetErrorCode();
}

static uint16_t ModbusRtu_Crc16(const uint8_t *data, uint8_t length)
{
  uint16_t crc = 0xFFFFU;
  uint8_t index = 0U;
  uint8_t bit = 0U;

  for (index = 0U; index < length; ++index)
  {
    crc ^= data[index];
    for (bit = 0U; bit < 8U; ++bit)
    {
      if ((crc & 0x0001U) != 0U)
      {
        crc = (crc >> 1U) ^ 0xA001U;
      }
      else
      {
        crc >>= 1U;
      }
    }
  }

  return crc;
}

static uint16_t ModbusRtu_ReadU16(const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static uint16_t ModbusRtu_ReadLeU16(const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[1] << 8U) | data[0]);
}

static void ModbusRtu_WriteU16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value >> 8U);
  data[1] = (uint8_t)(value & 0xFFU);
}

static void ModbusRtu_SendResponse(const uint8_t *data, uint8_t length)
{
  uint8_t response[MODBUS_RTU_MAX_FRAME_SIZE];
  uint16_t crc = 0U;

  if ((data == NULL) || (length > (MODBUS_RTU_MAX_FRAME_SIZE - 2U)))
  {
    return;
  }

  memcpy(response, data, length);
  crc = ModbusRtu_Crc16(response, length);
  response[length] = (uint8_t)(crc & 0xFFU);
  response[length + 1U] = (uint8_t)(crc >> 8U);
  Rs485Ll_Send(response, (size_t)length + 2U);
}

static void ModbusRtu_SendException(uint8_t slave_address, uint8_t function, uint8_t exception_code)
{
  uint8_t response[3];

  response[0] = slave_address;
  response[1] = (uint8_t)(function | 0x80U);
  response[2] = exception_code;
  ModbusRtu_SendResponse(response, 3U);
}

static void ModbusRtu_DebugPrintFrame(const char *prefix, const uint8_t *data, uint8_t length)
{
  uint8_t index = 0U;

  DEBUG_LOG("%s len=%u:", prefix, (unsigned int)length);
  for (index = 0U; index < length; ++index)
  {
    DEBUG_LOG(" %02X", data[index]);
  }
  DEBUG_LOG("\r\n");
}
