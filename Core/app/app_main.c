#include "app_main.h"

#include "main.h"

void app_init(void)
{
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, GPIO_PIN_RESET);
}

void app_main(void)
{
  /* Temporary heartbeat to confirm the app scheduler is running. */
  HAL_GPIO_TogglePin(LED_TEST_GPIO_Port, LED_TEST_Pin);
  HAL_Delay(500);
}
