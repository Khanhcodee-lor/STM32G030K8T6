#include "analog_input.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

#define ADS1115_ADDRESS_MIN             0x48U
#define ADS1115_ADDRESS_MAX             0x4BU
#define ADS1115_REGISTER_CONVERSION     0x00U
#define ADS1115_REGISTER_CONFIG         0x01U
#define ADS1115_CONFIG_OS_START         0x8000U
#define ADS1115_CONFIG_MUX_AIN0_GND     0x4000U
#define ADS1115_CONFIG_MODE_SINGLE_SHOT 0x0100U
#define ADS1115_CONFIG_DR_860SPS        0x00E0U
#define ADS1115_CONFIG_COMP_DISABLE     0x0003U

typedef enum
{
  ANALOG_INPUT_STATE_IDLE = 0,
  ANALOG_INPUT_STATE_WAITING_CONVERSION
} AnalogInputState;

extern I2C_HandleTypeDef hi2c2;

static uint8_t s_ads1115_address;
static uint8_t s_current_channel;
static uint8_t s_sampled_channel_mask;
static uint32_t s_state_tick_ms;
static uint16_t s_adc_code_values[MODBUS_MAP_AI_CHANNEL_COUNT];
static uint16_t s_board_raw_values[MODBUS_MAP_AI_CHANNEL_COUNT];
static uint16_t s_raw_values[MODBUS_MAP_AI_CHANNEL_COUNT];
static uint16_t s_scaled_mv_values[MODBUS_MAP_AI_CHANNEL_COUNT];
static uint16_t s_status_register;
static uint16_t s_error_code;
static AnalogInputState s_state;

static uint16_t AnalogInput_BuildConfig(uint8_t channel);
static bool AnalogInput_WriteRegister(uint8_t reg, uint16_t value);
static bool AnalogInput_ReadRegister(uint8_t reg, uint16_t *value);
static bool AnalogInput_StartConversion(uint8_t channel);
static bool AnalogInput_FinishConversion(uint8_t channel);
static bool AnalogInput_ReadSpecInputRegister(uint16_t address, uint16_t *value);
static bool AnalogInput_ReadDebugInputRegister(uint16_t address, uint16_t *value);
/* board raw = normalized raw in the board analog domain, before mapping to the public spec. */
static uint16_t AnalogInput_ConvertCodeToBoardRaw(uint16_t positive_code);
static uint16_t AnalogInput_ConvertBoardRawToSpecRaw(uint16_t board_raw_value);
static uint16_t AnalogInput_ConvertBoardRawToScaledMv(uint16_t board_raw_value);
static void AnalogInput_RecomputeStatus(void);
static uint8_t AnalogInput_GetMuxConfig(uint8_t channel);

void AnalogInput_Init(uint8_t ads1115_address)
{
  (void)memset(s_adc_code_values, 0, sizeof(s_adc_code_values));
  (void)memset(s_board_raw_values, 0, sizeof(s_board_raw_values));
  (void)memset(s_raw_values, 0, sizeof(s_raw_values));
  (void)memset(s_scaled_mv_values, 0, sizeof(s_scaled_mv_values));

  s_current_channel = 0U;
  s_sampled_channel_mask = 0U;
  s_state_tick_ms = HAL_GetTick();
  s_status_register = 0U;
  s_error_code = APP_ERROR_CODE_NONE;
  s_state = ANALOG_INPUT_STATE_IDLE;

  if ((ads1115_address < ADS1115_ADDRESS_MIN) || (ads1115_address > ADS1115_ADDRESS_MAX))
  {
    s_ads1115_address = 0U;
    s_error_code = APP_ERROR_CODE_AI_ADC_FAULT;
    return;
  }

  s_ads1115_address = ads1115_address;
}

void AnalogInput_Process(void)
{
  if (s_ads1115_address == 0U)
  {
    return;
  }

  if (s_state == ANALOG_INPUT_STATE_IDLE)
  {
    if ((s_error_code != APP_ERROR_CODE_NONE) &&
        ((HAL_GetTick() - s_state_tick_ms) < APP_AI_ADS1115_RETRY_DELAY_MS))
    {
      return;
    }

    if (AnalogInput_StartConversion(s_current_channel))
    {
      s_state = ANALOG_INPUT_STATE_WAITING_CONVERSION;
      s_state_tick_ms = HAL_GetTick();
    }
    else
    {
      s_error_code = APP_ERROR_CODE_AI_ADC_FAULT;
      s_status_register &= (uint16_t)~APP_STATUS_MODULE_READY_BIT;
      s_state_tick_ms = HAL_GetTick();
    }

    return;
  }

  if ((HAL_GetTick() - s_state_tick_ms) < APP_AI_ADS1115_CONVERSION_WAIT_MS)
  {
    return;
  }

  if (AnalogInput_FinishConversion(s_current_channel))
  {
    s_sampled_channel_mask |= (uint8_t)(1U << s_current_channel);
    s_current_channel = (uint8_t)((s_current_channel + 1U) % MODBUS_MAP_AI_CHANNEL_COUNT);
    s_error_code = APP_ERROR_CODE_NONE;
    AnalogInput_RecomputeStatus();
  }
  else
  {
    s_error_code = APP_ERROR_CODE_AI_ADC_FAULT;
    s_status_register &= (uint16_t)~APP_STATUS_MODULE_READY_BIT;
  }

  s_state = ANALOG_INPUT_STATE_IDLE;
  s_state_tick_ms = HAL_GetTick();
}

bool AnalogInput_ReadInputRegister(uint16_t address, uint16_t *value)
{
  if (AnalogInput_ReadSpecInputRegister(address, value))
  {
    return true;
  }

  return AnalogInput_ReadDebugInputRegister(address, value);
}

uint16_t AnalogInput_GetStatus(void)
{
  return s_status_register;
}

uint16_t AnalogInput_GetErrorCode(void)
{
  return s_error_code;
}

static bool AnalogInput_ReadSpecInputRegister(uint16_t address, uint16_t *value)
{
  if (value == NULL)
  {
    return false;
  }

  if ((uint16_t)(address - MODBUS_MAP_INPUT_REG_AI_RAW_BASE) < MODBUS_MAP_AI_CHANNEL_COUNT)
  {
    *value = s_raw_values[address - MODBUS_MAP_INPUT_REG_AI_RAW_BASE];
    return true;
  }

  if ((uint16_t)(address - MODBUS_MAP_INPUT_REG_AI_SCALED_BASE) < MODBUS_MAP_AI_CHANNEL_COUNT)
  {
    *value = s_scaled_mv_values[address - MODBUS_MAP_INPUT_REG_AI_SCALED_BASE];
    return true;
  }

  return false;
}

static bool AnalogInput_ReadDebugInputRegister(uint16_t address, uint16_t *value)
{
  if (value == NULL)
  {
    return false;
  }

  if ((uint16_t)(address - MODBUS_MAP_INPUT_REG_AI_BOARD_RAW_BASE) < MODBUS_MAP_AI_CHANNEL_COUNT)
  {
    *value = s_board_raw_values[address - MODBUS_MAP_INPUT_REG_AI_BOARD_RAW_BASE];
    return true;
  }

  if ((uint16_t)(address - MODBUS_MAP_INPUT_REG_AI_ADC_CODE_BASE) < MODBUS_MAP_AI_CHANNEL_COUNT)
  {
    *value = s_adc_code_values[address - MODBUS_MAP_INPUT_REG_AI_ADC_CODE_BASE];
    return true;
  }

  return false;
}

static uint16_t AnalogInput_BuildConfig(uint8_t channel)
{
  return (uint16_t)(ADS1115_CONFIG_OS_START |
                    ((uint16_t)AnalogInput_GetMuxConfig(channel) << 12U) |
                    APP_AI_ADS1115_PGA_CONFIG |
                    ADS1115_CONFIG_MODE_SINGLE_SHOT |
                    ADS1115_CONFIG_DR_860SPS |
                    ADS1115_CONFIG_COMP_DISABLE);
}

static bool AnalogInput_WriteRegister(uint8_t reg, uint16_t value)
{
  uint8_t frame[3];

  frame[0] = reg;
  frame[1] = (uint8_t)(value >> 8U);
  frame[2] = (uint8_t)(value & 0xFFU);

  return HAL_I2C_Master_Transmit(
             &hi2c2,
             (uint16_t)(s_ads1115_address << 1U),
             frame,
             sizeof(frame),
             APP_AI_ADS1115_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool AnalogInput_ReadRegister(uint8_t reg, uint16_t *value)
{
  uint8_t reg_address = reg;
  uint8_t data[2];

  if (value == NULL)
  {
    return false;
  }

  if (HAL_I2C_Master_Transmit(
          &hi2c2,
          (uint16_t)(s_ads1115_address << 1U),
          &reg_address,
          1U,
          APP_AI_ADS1115_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }

  if (HAL_I2C_Master_Receive(
          &hi2c2,
          (uint16_t)(s_ads1115_address << 1U),
          data,
          sizeof(data),
          APP_AI_ADS1115_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }

  *value = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
  return true;
}

static bool AnalogInput_StartConversion(uint8_t channel)
{
  return AnalogInput_WriteRegister(ADS1115_REGISTER_CONFIG, AnalogInput_BuildConfig(channel));
}

static bool AnalogInput_FinishConversion(uint8_t channel)
{
  uint16_t conversion_register = 0U;
  int16_t signed_code = 0;
  uint16_t positive_code = 0U;
  uint16_t board_raw_value = 0U;

  if (!AnalogInput_ReadRegister(ADS1115_REGISTER_CONVERSION, &conversion_register))
  {
    return false;
  }

  signed_code = (int16_t)conversion_register;
  if (signed_code > 0)
  {
    positive_code = (uint16_t)signed_code;
  }

  s_adc_code_values[channel] = positive_code;
  board_raw_value = AnalogInput_ConvertCodeToBoardRaw(positive_code);
  s_board_raw_values[channel] = board_raw_value;
  s_raw_values[channel] = AnalogInput_ConvertBoardRawToSpecRaw(board_raw_value);
  s_scaled_mv_values[channel] = AnalogInput_ConvertBoardRawToScaledMv(board_raw_value);
  return true;
}

static uint16_t AnalogInput_ConvertCodeToBoardRaw(uint16_t positive_code)
{
  uint32_t board_raw_value = positive_code;

  if (board_raw_value > APP_AI_ADS1115_POSITIVE_CODE_MAX)
  {
    board_raw_value = APP_AI_ADS1115_POSITIVE_CODE_MAX;
  }

  board_raw_value = (board_raw_value * APP_AI_RAW_MAX + (APP_AI_ADS1115_POSITIVE_CODE_MAX / 2U)) /
                    APP_AI_ADS1115_POSITIVE_CODE_MAX;

  if (board_raw_value > APP_AI_RAW_MAX)
  {
    board_raw_value = APP_AI_RAW_MAX;
  }

  return (uint16_t)board_raw_value;
}

static uint16_t AnalogInput_ConvertBoardRawToSpecRaw(uint16_t board_raw_value)
{
  uint32_t spec_raw_value = board_raw_value;

  /* Ép board raw về dải raw tài liệu yêu cầu: 0..4095 tại 0..10V. */
  if (spec_raw_value >= APP_AI_CALIBRATED_RAW_AT_10V)
  {
    return APP_AI_RAW_MAX;
  }

  spec_raw_value = (spec_raw_value * APP_AI_RAW_MAX + (APP_AI_CALIBRATED_RAW_AT_10V / 2U)) /
                   APP_AI_CALIBRATED_RAW_AT_10V;

  if (spec_raw_value > APP_AI_RAW_MAX)
  {
    spec_raw_value = APP_AI_RAW_MAX;
  }

  return (uint16_t)spec_raw_value;
}

  static uint16_t AnalogInput_ConvertBoardRawToScaledMv(uint16_t board_raw_value)
  {
    uint32_t scaled_mv = board_raw_value;

    /*
    * Follow the original application note example:
    * AIx_SCALED = 5000 means Vin = 5.000V, so the register is stored in mV.
    * Giá trị đầu vào của hàm này vẫn là board raw, chưa phải spec raw.
    AI_RAW là giá trị raw chuẩn hóa theo spec 12-bit của module, không phải mã ADC gốc từ ADS1115.
    */
    if (scaled_mv >= APP_AI_CALIBRATED_RAW_AT_10V)
    {
      return APP_AI_SCALED_MAX_MV;
    }

    scaled_mv = (scaled_mv * APP_AI_SCALED_MAX_MV + (APP_AI_CALIBRATED_RAW_AT_10V / 2U)) /
                APP_AI_CALIBRATED_RAW_AT_10V;

    if (scaled_mv > APP_AI_SCALED_MAX_MV)
    {
      scaled_mv = APP_AI_SCALED_MAX_MV;
    }

    return (uint16_t)scaled_mv;
  }

static void AnalogInput_RecomputeStatus(void)
{
  uint8_t index = 0U;
  uint16_t status = 0U;

  if (s_sampled_channel_mask == ((1U << MODBUS_MAP_AI_CHANNEL_COUNT) - 1U))
  {
    status |= APP_STATUS_MODULE_READY_BIT;
  }

  for (index = 0U; index < MODBUS_MAP_AI_CHANNEL_COUNT; ++index)
  {
    if (s_raw_values[index] >= APP_AI_RAW_MAX)
    {
      status |= APP_STATUS_AI_OVERRANGE_BIT;
      break;
    }
  }

  s_status_register = status;
}

static uint8_t AnalogInput_GetMuxConfig(uint8_t channel)
{
  return (uint8_t)(0x04U + (channel & 0x03U));
}
