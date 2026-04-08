#include "modbus_rtu.h"

#include "analog_input.h"
#include "app_config.h"
#include "debug_log.h"
#include "device_config.h"
#include "main.h"
#include "rs485_ll.h"
#include <string.h>

#define MODBUS_RTU_MAX_FRAME_SIZE 32U
#define MODBUS_FUNC_READ_HOLDING_REGISTERS 0x03U
#define MODBUS_FUNC_READ_INPUT_REGISTERS   0x04U
#define MODBUS_FUNC_WRITE_SINGLE_REGISTER  0x06U
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION  0x01U
#define MODBUS_EXCEPTION_ILLEGAL_ADDRESS   0x02U
#define MODBUS_EXCEPTION_ILLEGAL_VALUE     0x03U
#define MODBUS_EXCEPTION_DEVICE_FAILURE    0x04U
#define MODBUS_RTU_MAX_READ_REGISTERS      ((MODBUS_RTU_MAX_FRAME_SIZE - 5U) / 2U)

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
  uint16_t register_address = 0U;
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

      register_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      if ((register_address != APP_MODBUS_REG_SLAVE_ADDRESS) ||
          (ModbusRtu_ReadU16(&s_rx_buffer[4]) != 1U))
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_ADDRESS);
        return;
      }

      {
        uint8_t response[7];
        response[0] = slave_address;
        response[1] = function;
        response[2] = 2U;
        ModbusRtu_WriteU16(&response[3], DeviceConfig_GetModbusAddress());
        ModbusRtu_SendResponse(response, 5U);
      }
      break;

    case MODBUS_FUNC_READ_INPUT_REGISTERS:
      if (s_rx_length != 8U)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
        return;
      }

      register_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
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
          if (!AnalogInput_ReadInputRegister((uint16_t)(register_address + index), &register_value))
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

      register_address = ModbusRtu_ReadU16(&s_rx_buffer[2]);
      if (register_address != APP_MODBUS_REG_SLAVE_ADDRESS)
      {
        ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_ADDRESS);
        return;
      }

      {
        uint16_t requested_address = ModbusRtu_ReadU16(&s_rx_buffer[4]);
        if ((requested_address < APP_MODBUS_ADDRESS_MIN) ||
            (requested_address > APP_MODBUS_ADDRESS_MAX))
        {
          ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_VALUE);
          return;
        }

        ModbusRtu_SendResponse(s_rx_buffer, 6U);
        s_pending_address = (uint8_t)requested_address;
        s_pending_address_valid = 1U;
        s_pending_apply_tick_ms = HAL_GetTick();
      }
      break;

    default:
      ModbusRtu_SendException(slave_address, function, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
      break;
  }
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
