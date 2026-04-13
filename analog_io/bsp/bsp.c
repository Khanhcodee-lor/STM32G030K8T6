#include "bsp.h"

#include "app_config.h"
#include "device_config.h"
#include "main.h"
#include "pal.h"

I2C_HandleTypeDef hi2c2;

#define BSP_IWDG_KEY_WRITE_ACCESS 0x5555U
#define BSP_IWDG_KEY_RELOAD       0xAAAAU
#define BSP_IWDG_KEY_ENABLE       0xCCCCU
#define BSP_IWDG_PRESCALER_DIV    64U
#define BSP_IWDG_PRESCALER_CODE   4U
#define BSP_IWDG_LSI_FREQ_HZ      32000U

static uint32_t s_led_tick_ms;
static BspResetCause s_reset_cause = BSP_RESET_CAUSE_UNKNOWN;
static uint8_t s_watchdog_enabled;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void Bsp_ReadAndClearResetCause(void);
#if (APP_WATCHDOG_ENABLE != 0U)
static void Bsp_EnableWatchdogDebugFreeze(void);
#endif

void bsp_init(void)
{
  Bsp_ReadAndClearResetCause();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C2_Init();
  pal_debug_init();
  DeviceConfig_Init();
  HAL_GPIO_WritePin(LED_TEST_GPIO_Port, LED_TEST_Pin, GPIO_PIN_RESET);
  s_led_tick_ms = HAL_GetTick();
}

void bsp_heartbeat_process(void)
{
  if ((HAL_GetTick() - s_led_tick_ms) >= APP_LED_HEARTBEAT_PERIOD_MS)
  {
    HAL_GPIO_TogglePin(LED_TEST_GPIO_Port, LED_TEST_Pin);
    s_led_tick_ms = HAL_GetTick();
  }
}

uint8_t bsp_get_modbus_address(void)
{
  return DeviceConfig_GetModbusAddress();
}

int bsp_set_modbus_address(uint8_t address)
{
  uint8_t previous_address = DeviceConfig_GetModbusAddress();

  if (!DeviceConfig_SetModbusAddress(address))
  {
    return 0;
  }

  if (!DeviceConfig_Save())
  {
    (void)DeviceConfig_SetModbusAddress(previous_address);
    return 0;
  }

  return 1;
}

BspResetCause bsp_get_reset_cause(void)
{
  return s_reset_cause;
}

const char *bsp_get_reset_cause_name(void)
{
  switch (s_reset_cause)
  {
    case BSP_RESET_CAUSE_POWER:
      return "power";

    case BSP_RESET_CAUSE_PIN:
      return "pin";

    case BSP_RESET_CAUSE_SOFTWARE:
      return "software";

    case BSP_RESET_CAUSE_IWDG:
      return "watchdog_iwdg";

    case BSP_RESET_CAUSE_WWDG:
      return "watchdog_wwdg";

    case BSP_RESET_CAUSE_LOW_POWER:
      return "low_power";

    default:
      return "unknown";
  }
}

void bsp_watchdog_init(void)
{
#if (APP_WATCHDOG_ENABLE != 0U)
  uint32_t reload_value = 0U;

  Bsp_EnableWatchdogDebugFreeze();

  reload_value = ((APP_WATCHDOG_TIMEOUT_MS * BSP_IWDG_LSI_FREQ_HZ) +
                  (BSP_IWDG_PRESCALER_DIV * 1000U) - 1U) /
                 (BSP_IWDG_PRESCALER_DIV * 1000U);
  if (reload_value == 0U)
  {
    reload_value = 1U;
  }
  else if (reload_value > 0x1000U)
  {
    reload_value = 0x1000U;
  }

  IWDG->KR = BSP_IWDG_KEY_WRITE_ACCESS;
  IWDG->PR = BSP_IWDG_PRESCALER_CODE;
  IWDG->RLR = reload_value - 1U;
  while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U)
  {
  }
  IWDG->KR = BSP_IWDG_KEY_RELOAD;
  IWDG->KR = BSP_IWDG_KEY_ENABLE;

  s_watchdog_enabled = 1U;
#else
  s_watchdog_enabled = 0U;
#endif
}

void bsp_watchdog_kick(void)
{
  if (s_watchdog_enabled != 0U)
  {
    IWDG->KR = BSP_IWDG_KEY_RELOAD;
  }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00503D58;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void Bsp_ReadAndClearResetCause(void)
{
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_IWDG;
  }
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_WWDG;
  }
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_SOFTWARE;
  }
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PWRRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_POWER;
  }
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_PIN;
  }
  else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
  {
    s_reset_cause = BSP_RESET_CAUSE_LOW_POWER;
  }
  else
  {
    s_reset_cause = BSP_RESET_CAUSE_UNKNOWN;
  }

  __HAL_RCC_CLEAR_RESET_FLAGS();
}

#if (APP_WATCHDOG_ENABLE != 0U)
static void Bsp_EnableWatchdogDebugFreeze(void)
{
#if (APP_WATCHDOG_FREEZE_IN_DEBUG != 0U)
  __HAL_RCC_DBGMCU_CLK_ENABLE();
#if defined(__HAL_DBGMCU_FREEZE_IWDG)
  __HAL_DBGMCU_FREEZE_IWDG();
#endif
#endif
}
#endif
