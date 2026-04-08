#include "app_main.h"

#include "app_config.h"
#include "device_config.h"
#include "debug_log.h"
#include "i2c_bus_scan.h"
#include "main.h"
#include "modbus_rtu.h"
#include "rs485_ll.h"

static uint32_t s_led_tick_ms;

static void App_LogI2cScanReport(const I2cBusScanReport *report);

void app_init(void)
{
  I2cBusScanReport i2c_scan_report;

  DebugLog_Init();
  DeviceConfig_Init();
  Rs485Ll_Init(APP_RS485_TEST_BAUDRATE);
  ModbusRtu_Init();
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, GPIO_PIN_RESET);
  DEBUG_LOG("boot ok\r\n");
  DEBUG_LOG("modbus addr: %u\r\n", DeviceConfig_GetModbusAddress());
  DEBUG_LOG("rs485/modbus ready @ %lu\r\n", (unsigned long)APP_RS485_TEST_BAUDRATE);
  DEBUG_LOG("i2c2 scan range: 0x08..0x77\r\n");
  I2cBusScan_Run(&i2c_scan_report);
  App_LogI2cScanReport(&i2c_scan_report);
  s_led_tick_ms = HAL_GetTick();
}

void app_main(void)
{
  if ((HAL_GetTick() - s_led_tick_ms) >= APP_LED_HEARTBEAT_PERIOD_MS)
  {
    HAL_GPIO_TogglePin(LED_TEST_GPIO_Port, LED_TEST_Pin);
    s_led_tick_ms = HAL_GetTick();
  }

  ModbusRtu_Poll(DeviceConfig_GetModbusAddress());
}

static void App_LogI2cScanReport(const I2cBusScanReport *report)
{
  uint8_t i;
  uint8_t listed_count;

  if (report == NULL)
  {
    return;
  }

  listed_count = report->found_count;
  if (listed_count > I2C_BUS_SCAN_MAX_FOUND_DEVICES)
  {
    listed_count = I2C_BUS_SCAN_MAX_FOUND_DEVICES;
  }

  if (report->found_count == 0U)
  {
    DEBUG_LOG("i2c2 scan: no ack\r\n");
  }
  else
  {
    DEBUG_LOG("i2c2 devices: %u\r\n", report->found_count);
    for (i = 0U; i < listed_count; ++i)
    {
      DEBUG_LOG("i2c2 ack @ 0x%02X\r\n", report->found_addresses[i]);
    }

    if (report->overflow != 0U)
    {
      DEBUG_LOG("i2c2 scan: result truncated after %u addresses\r\n", I2C_BUS_SCAN_MAX_FOUND_DEVICES);
    }
  }

  if (report->ads1115_found != 0U)
  {
    DEBUG_LOG("ads1115 ack @ 0x%02X\r\n", report->ads1115_address);
  }
  else
  {
    DEBUG_LOG("ads1115 ack: not found in 0x48..0x4B\r\n");
  }

  if (report->mcp4728_found != 0U)
  {
    DEBUG_LOG("mcp4728 ack @ 0x%02X\r\n", report->mcp4728_address);
  }
  else
  {
    DEBUG_LOG("mcp4728 ack: not found in 0x60..0x67\r\n");
  }
}
