#include "i2c_bus_scan.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

#define I2C_BUS_SCAN_ADDR_MIN       0x08U
#define I2C_BUS_SCAN_ADDR_MAX       0x77U
#define I2C_BUS_SCAN_ADS1115_MIN    0x48U
#define I2C_BUS_SCAN_ADS1115_MAX    0x4BU
#define I2C_BUS_SCAN_MCP4728_MIN    0x60U
#define I2C_BUS_SCAN_MCP4728_MAX    0x67U

extern I2C_HandleTypeDef hi2c2;

static uint8_t I2cBusScan_IsDeviceReady(uint8_t address);
static void I2cBusScan_RecordDevice(I2cBusScanReport *report, uint8_t address);
static void I2cBusScan_UpdateMatches(I2cBusScanReport *report, uint8_t address);

void I2cBusScan_Run(I2cBusScanReport *report)
{
  uint8_t address;

  if (report == NULL)
  {
    return;
  }

  (void)memset(report, 0, sizeof(*report));

  for (address = I2C_BUS_SCAN_ADDR_MIN; address <= I2C_BUS_SCAN_ADDR_MAX; ++address)
  {
    if (I2cBusScan_IsDeviceReady(address) == 0U)
    {
      continue;
    }

    I2cBusScan_RecordDevice(report, address);
    I2cBusScan_UpdateMatches(report, address);
  }
}

static uint8_t I2cBusScan_IsDeviceReady(uint8_t address)
{
  HAL_StatusTypeDef status;

  status = HAL_I2C_IsDeviceReady(
      &hi2c2,
      (uint16_t)(address << 1),
      APP_I2C_SCAN_READY_TRIALS,
      APP_I2C_SCAN_TIMEOUT_MS);

  return (status == HAL_OK) ? 1U : 0U;
}

static void I2cBusScan_RecordDevice(I2cBusScanReport *report, uint8_t address)
{
  if (report->found_count < I2C_BUS_SCAN_MAX_FOUND_DEVICES)
  {
    report->found_addresses[report->found_count] = address;
  }
  else
  {
    report->overflow = 1U;
  }

  if (report->found_count < UINT8_MAX)
  {
    ++report->found_count;
  }
}

static void I2cBusScan_UpdateMatches(I2cBusScanReport *report, uint8_t address)
{
  if ((report->ads1115_found == 0U) &&
      (address >= I2C_BUS_SCAN_ADS1115_MIN) &&
      (address <= I2C_BUS_SCAN_ADS1115_MAX))
  {
    report->ads1115_found = 1U;
    report->ads1115_address = address;
  }

  if ((report->mcp4728_found == 0U) &&
      (address >= I2C_BUS_SCAN_MCP4728_MIN) &&
      (address <= I2C_BUS_SCAN_MCP4728_MAX))
  {
    report->mcp4728_found = 1U;
    report->mcp4728_address = address;
  }
}
