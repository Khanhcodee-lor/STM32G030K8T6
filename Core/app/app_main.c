#include "app_main.h"

#include "app_config.h"
#include "debug_log.h"
#include "main.h"
#include "rs485_ll.h"
#include <string.h>

static uint32_t s_led_tick_ms;

void app_init(void)
{
  DebugLog_Init();
  Rs485Ll_Init(APP_RS485_TEST_BAUDRATE);
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, GPIO_PIN_RESET);
  DEBUG_LOG("boot ok\r\n");
  Rs485Ll_Send((const uint8_t *)g_app_rs485_boot_banner, strlen(g_app_rs485_boot_banner));
  DEBUG_LOG("rs485 ready @ %lu\r\n", (unsigned long)APP_RS485_TEST_BAUDRATE);
  s_led_tick_ms = HAL_GetTick();
}

void app_main(void)
{
  uint8_t rx_byte;

  if ((HAL_GetTick() - s_led_tick_ms) >= APP_LED_HEARTBEAT_PERIOD_MS)
  {
    HAL_GPIO_TogglePin(LED_TEST_GPIO_Port, LED_TEST_Pin);
    s_led_tick_ms = HAL_GetTick();
  }

  if (Rs485Ll_ReadByte(&rx_byte))
  {
    Rs485Ll_Send(&rx_byte, 1U);
    DEBUG_LOG("rs485 rx: 0x%02X\r\n", rx_byte);
  }
}
