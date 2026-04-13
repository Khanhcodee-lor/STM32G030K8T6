#include "pal.h"

#include "debug_log.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

void pal_debug_init(void)
{
  DebugLog_Init();
}

void pal_logf(const char *fmt, ...)
{
  char buffer[128];
  va_list args;

  va_start(args, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  DebugLog_Printf("%s", buffer);
}

uint32_t pal_millis(void)
{
  return HAL_GetTick();
}

void pal_delay_ms(uint32_t delay_ms)
{
  HAL_Delay(delay_ms);
}

void pal_led_set(uint8_t state)
{
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, (state != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void pal_led_toggle(void)
{
  HAL_GPIO_TogglePin(LED_TEST_GPIO_Port, LED_TEST_Pin);
}
