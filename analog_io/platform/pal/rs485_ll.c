#include "rs485_ll.h"

#include "app_config.h"
#include "main.h"

#define RS485_LL_RX_BUFFER_MASK (APP_RS485_RX_BUFFER_SIZE - 1U)

static void Rs485Ll_SetTransmitMode(void);
static void Rs485Ll_SetReceiveMode(void);
static void Rs485Ll_ResetRxBuffer(void);
static void Rs485Ll_PushRxByte(uint8_t byte);

static volatile uint8_t s_rx_buffer[APP_RS485_RX_BUFFER_SIZE];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;

void Rs485Ll_Init(uint32_t baudrate)
{
  RCC->APBENR2 |= RCC_APBENR2_USART1EN;

  USART1->CR1 = 0U;
  USART1->CR2 = 0U;
  USART1->CR3 = 0U;
  USART1->BRR = 16000000UL / baudrate;
  USART1->ICR = USART_ICR_TCCF | USART_ICR_ORECF;

  Rs485Ll_ResetRxBuffer();
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE;

  HAL_NVIC_SetPriority(USART1_IRQn, 1U, 0U);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  Rs485Ll_SetReceiveMode();
}

void Rs485Ll_Send(const uint8_t *data, size_t length)
{
  size_t index;

  if ((data == NULL) || (length == 0U))
  {
    return;
  }

  Rs485Ll_SetTransmitMode();
  USART1->ICR = USART_ICR_TCCF;

  for (index = 0U; index < length; ++index)
  {
    while ((USART1->ISR & USART_ISR_TXE_TXFNF) == 0U)
    {
    }

    USART1->TDR = (uint32_t)data[index] & USART_TDR_TDR;
  }

  while ((USART1->ISR & USART_ISR_TC) == 0U)
  {
  }

  Rs485Ll_SetReceiveMode();
}

bool Rs485Ll_ReadByte(uint8_t *byte)
{
  uint8_t tail = 0U;

  if ((byte == NULL) || (s_rx_head == s_rx_tail))
  {
    return false;
  }

  tail = s_rx_tail;
  *byte = s_rx_buffer[tail];
  s_rx_tail = (uint8_t)((tail + 1U) & RS485_LL_RX_BUFFER_MASK);
  return true;
}

void Rs485Ll_IrqHandler(void)
{
  if ((USART1->ISR & USART_ISR_ORE) != 0U)
  {
    USART1->ICR = USART_ICR_ORECF;
  }

  while ((USART1->ISR & USART_ISR_RXNE_RXFNE) != 0U)
  {
    Rs485Ll_PushRxByte((uint8_t)(USART1->RDR & USART_RDR_RDR));
  }
}

static void Rs485Ll_SetTransmitMode(void)
{
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET);
}

static void Rs485Ll_SetReceiveMode(void)
{
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);
}

static void Rs485Ll_ResetRxBuffer(void)
{
  s_rx_head = 0U;
  s_rx_tail = 0U;
}

static void Rs485Ll_PushRxByte(uint8_t byte)
{
  uint8_t next_head = (uint8_t)((s_rx_head + 1U) & RS485_LL_RX_BUFFER_MASK);

  if (next_head == s_rx_tail)
  {
    return;
  }

  s_rx_buffer[s_rx_head] = byte;
  s_rx_head = next_head;
}
