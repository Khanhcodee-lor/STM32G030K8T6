#include "calibration.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t crc32;
  uint32_t reserved;
  CalibrationData data;
} CalibrationFlashRecord;

static CalibrationData s_calibration_data;
static uint8_t s_factory_data_loaded;

static const CalibrationFlashRecord *Calibration_GetFlashRecord(void);
static bool Calibration_IsDataValid(const CalibrationData *data);
static uint32_t Calibration_ComputeCrc32(const uint8_t *data, size_t length);

void Calibration_Init(void)
{
  const CalibrationFlashRecord *record = Calibration_GetFlashRecord();
  uint32_t expected_crc32 = 0U;

  s_factory_data_loaded = 0U;

  if ((record->magic == APP_CALIBRATION_FLASH_MAGIC) &&
      (record->version == APP_CALIBRATION_FLASH_VERSION) &&
      Calibration_IsDataValid(&record->data))
  {
    expected_crc32 = Calibration_ComputeCrc32((const uint8_t *)&record->data, sizeof(record->data));
    if (record->crc32 == expected_crc32)
    {
      s_calibration_data = record->data;
      s_factory_data_loaded = 1U;
      return;
    }
  }

  Calibration_LoadDefaults(&s_calibration_data);
}

bool Calibration_IsFactoryDataLoaded(void)
{
  return (s_factory_data_loaded != 0U);
}

const CalibrationData *Calibration_Get(void)
{
  return &s_calibration_data;
}

void Calibration_LoadDefaults(CalibrationData *data)
{
  uint8_t index = 0U;

  if (data == NULL)
  {
    return;
  }

  (void)memset(data, 0, sizeof(*data));

  for (index = 0U; index < MODBUS_MAP_AI_CHANNEL_COUNT; ++index)
  {
    data->ai_board_raw_zero[index] = APP_AI_DEFAULT_BOARD_RAW_ZERO_AT_0V;
    data->ai_board_raw_full_scale[index] = APP_AI_CALIBRATED_RAW_AT_10V;
  }

  for (index = 0U; index < MODBUS_MAP_AO_CHANNEL_COUNT; ++index)
  {
    data->ao_voltage_min_code[index] = APP_AO_MODE_VOLTAGE_DAC_MIN_CODE;
    data->ao_voltage_max_code[index] = APP_AO_MODE_VOLTAGE_DAC_MAX_CODE;
    data->ao_current_min_code[index] = APP_AO_MODE_CURRENT_DAC_MIN_CODE;
    data->ao_current_max_code[index] = APP_AO_MODE_CURRENT_DAC_MAX_CODE;
  }
}

bool Calibration_Save(const CalibrationData *data)
{
  CalibrationFlashRecord record;
  FLASH_EraseInitTypeDef erase_init = {0};
  uint32_t page_error = 0U;
  uint32_t offset = 0U;
  uint64_t doubleword = 0U;

  if ((data == NULL) || !Calibration_IsDataValid(data))
  {
    return false;
  }

  record.magic = APP_CALIBRATION_FLASH_MAGIC;
  record.version = APP_CALIBRATION_FLASH_VERSION;
  record.reserved = 0U;
  record.data = *data;
  record.crc32 = Calibration_ComputeCrc32((const uint8_t *)&record.data, sizeof(record.data));

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    return false;
  }

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR |
                         FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR |
                         FLASH_FLAG_PGSERR | FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR);

  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.Banks = FLASH_BANK_1;
  erase_init.Page = APP_CALIBRATION_FLASH_PAGE_INDEX;
  erase_init.NbPages = 1U;

  if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
  {
    (void)HAL_FLASH_Lock();
    return false;
  }

  for (offset = 0U; offset < sizeof(record); offset += sizeof(doubleword))
  {
    (void)memcpy(&doubleword, ((const uint8_t *)&record) + offset, sizeof(doubleword));
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                          APP_CALIBRATION_FLASH_ADDRESS + offset,
                          doubleword) != HAL_OK)
    {
      (void)HAL_FLASH_Lock();
      return false;
    }
  }

  if (HAL_FLASH_Lock() != HAL_OK)
  {
    return false;
  }

  s_calibration_data = *data;
  s_factory_data_loaded = 1U;
  return true;
}

static const CalibrationFlashRecord *Calibration_GetFlashRecord(void)
{
  return (const CalibrationFlashRecord *)APP_CALIBRATION_FLASH_ADDRESS;
}

static bool Calibration_IsDataValid(const CalibrationData *data)
{
  uint8_t index = 0U;

  if (data == NULL)
  {
    return false;
  }

  for (index = 0U; index < MODBUS_MAP_AI_CHANNEL_COUNT; ++index)
  {
    if (data->ai_board_raw_zero[index] >= data->ai_board_raw_full_scale[index])
    {
      return false;
    }

    if (data->ai_board_raw_full_scale[index] > APP_AI_RAW_MAX)
    {
      return false;
    }
  }

  for (index = 0U; index < MODBUS_MAP_AO_CHANNEL_COUNT; ++index)
  {
    if ((data->ao_voltage_min_code[index] > data->ao_voltage_max_code[index]) ||
        (data->ao_current_min_code[index] > data->ao_current_max_code[index]) ||
        (data->ao_voltage_max_code[index] > APP_AO_SETPOINT_MAX) ||
        (data->ao_current_max_code[index] > APP_AO_SETPOINT_MAX))
    {
      return false;
    }
  }

  return true;
}

static uint32_t Calibration_ComputeCrc32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xFFFFFFFFUL;
  size_t index = 0U;
  uint8_t bit = 0U;

  if (data == NULL)
  {
    return 0U;
  }

  for (index = 0U; index < length; ++index)
  {
    crc ^= data[index];
    for (bit = 0U; bit < 8U; ++bit)
    {
      if ((crc & 1UL) != 0U)
      {
        crc = (crc >> 1U) ^ 0xEDB88320UL;
      }
      else
      {
        crc >>= 1U;
      }
    }
  }

  return crc;
}
