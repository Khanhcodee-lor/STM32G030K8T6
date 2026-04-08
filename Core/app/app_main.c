#include "app_main.h"

#include "app_config.h"
#include "device_config.h"
#include "debug_log.h"
#include "main.h"
#include "modbus_rtu.h"
#include "rs485_ll.h"

static uint32_t s_led_tick_ms;

void app_init(void)
{
  DebugLog_Init();
  DeviceConfig_Init();
  Rs485Ll_Init(APP_RS485_TEST_BAUDRATE);
  ModbusRtu_Init();
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, GPIO_PIN_RESET);
  DEBUG_LOG("boot ok\r\n");
  DEBUG_LOG("modbus addr: %u\r\n", DeviceConfig_GetModbusAddress());
  DEBUG_LOG("rs485/modbus ready @ %lu\r\n", (unsigned long)APP_RS485_TEST_BAUDRATE);
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
