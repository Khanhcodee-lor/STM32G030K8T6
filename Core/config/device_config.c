#include "device_config.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t modbus_address;
  uint32_t reserved;
} DeviceConfigFlashRecord;

static uint8_t s_modbus_address = APP_MODBUS_DEFAULT_ADDRESS;

static bool DeviceConfig_IsAddressValid(uint8_t address);
static const DeviceConfigFlashRecord *DeviceConfig_GetFlashRecord(void);

void DeviceConfig_Init(void)
{
  const DeviceConfigFlashRecord *record = DeviceConfig_GetFlashRecord();

  if ((record->magic == APP_CONFIG_FLASH_MAGIC) &&
      (record->version == APP_CONFIG_FLASH_VERSION) &&
      DeviceConfig_IsAddressValid((uint8_t)record->modbus_address))
  {
    s_modbus_address = (uint8_t)record->modbus_address;
    return;
  }

  s_modbus_address = APP_MODBUS_DEFAULT_ADDRESS;
}

uint8_t DeviceConfig_GetModbusAddress(void)
{
  return s_modbus_address;
}

bool DeviceConfig_SetModbusAddress(uint8_t address)
{
  if (!DeviceConfig_IsAddressValid(address))
  {
    return false;
  }

  s_modbus_address = address;
  return true;
}

bool DeviceConfig_Save(void)
{
  FLASH_EraseInitTypeDef erase_init = {0};
  uint32_t page_error = 0U;
  uint64_t flash_data[2];

  flash_data[0] = ((uint64_t)APP_CONFIG_FLASH_VERSION << 32) | APP_CONFIG_FLASH_MAGIC;
  flash_data[1] = (uint64_t)s_modbus_address;

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    return false;
  }

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR |
                         FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR |
                         FLASH_FLAG_PGSERR | FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR);

  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.Banks = FLASH_BANK_1;
  erase_init.Page = APP_CONFIG_FLASH_PAGE_INDEX;
  erase_init.NbPages = 1U;

  if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
  {
    (void)HAL_FLASH_Lock();
    return false;
  }

  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                        APP_CONFIG_FLASH_ADDRESS,
                        flash_data[0]) != HAL_OK)
  {
    (void)HAL_FLASH_Lock();
    return false;
  }

  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                        APP_CONFIG_FLASH_ADDRESS + 8U,
                        flash_data[1]) != HAL_OK)
  {
    (void)HAL_FLASH_Lock();
    return false;
  }

  return (HAL_FLASH_Lock() == HAL_OK);
}

static bool DeviceConfig_IsAddressValid(uint8_t address)
{
  return (address >= APP_MODBUS_ADDRESS_MIN) && (address <= APP_MODBUS_ADDRESS_MAX);
}

static const DeviceConfigFlashRecord *DeviceConfig_GetFlashRecord(void)
{
  return (const DeviceConfigFlashRecord *)APP_CONFIG_FLASH_ADDRESS;
}
