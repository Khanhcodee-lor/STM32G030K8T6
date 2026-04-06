#include "rs485_ll.h"

#include "main.h"

static void Rs485Ll_SetTransmitMode(void);
static void Rs485Ll_SetReceiveMode(void);

//Thiết lập UART giao tiếp với RS485
void Rs485Ll_Init(uint32_t baudrate)
{
  RCC->APBENR2 |= RCC_APBENR2_USART1EN; // Bật clock
  //reset cấu hình 
  USART1->CR1 = 0U;
  USART1->CR2 = 0U;
  USART1->CR3 = 0U;

  USART1->BRR = 16000000UL / baudrate;

  USART1->ICR = USART_ICR_TCCF | USART_ICR_ORECF; // Xóa cờ TC(Transmission Complete)/ORE (Overrun Error)
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // UE: bật USART, TE: bật truyền, RE: bật nhận

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

  for (index = 0; index < length; ++index)
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
  if (byte == NULL)
  {
    return false;
  }

  if ((USART1->ISR & USART_ISR_ORE) != 0U)
  {
    USART1->ICR = USART_ICR_ORECF;
  }

  if ((USART1->ISR & USART_ISR_RXNE_RXFNE) == 0U)
  {
    return false;
  }

  *byte = (uint8_t)(USART1->RDR & USART_RDR_RDR);
  return true;
}

static void Rs485Ll_SetTransmitMode(void)
{
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET);
}

static void Rs485Ll_SetReceiveMode(void)
{
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);
}
