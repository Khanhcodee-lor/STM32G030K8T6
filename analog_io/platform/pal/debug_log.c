#include "debug_log.h"

#include "app_config.h"
#include "main.h"
#include <string.h>

static void DebugLog_WriteByte(uint8_t byte);

void DebugLog_Init(void)
{
  uint32_t usart_clock_hz = HAL_RCC_GetPCLK1Freq();

  if (usart_clock_hz == 0U)
  {
    usart_clock_hz = 16000000UL;
  }

  RCC->APBENR1 |= RCC_APBENR1_USART2EN;

  USART2->CR1 = 0U;
  USART2->CR2 = 0U;
  USART2->CR3 = 0U;
  USART2->BRR = (usart_clock_hz + (APP_DEBUG_UART_BAUDRATE / 2U)) / APP_DEBUG_UART_BAUDRATE;
  USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void DebugLog_Printf(const char *fmt, ...)
{
  char buffer[128];
  va_list args;
  int length;
  size_t i;

  va_start(args, fmt);
  length = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (length <= 0)
  {
    return;
  }

  for (i = 0; i < strnlen(buffer, sizeof(buffer)); ++i)
  {
    DebugLog_WriteByte((uint8_t)buffer[i]);
  }
}

static void DebugLog_WriteByte(uint8_t byte)
{
  while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0U)
  {
  }

  USART2->TDR = (uint32_t)byte & USART_TDR_TDR;
}
