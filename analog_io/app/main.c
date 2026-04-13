#include "bsp.h"
#include "config.h"
#include "main.h"
#include "modbus_service.h"
#include "analog_io_service.h"
#include "pal.h"

int main(void)
{
  HAL_Init();
  bsp_init();

  pal_logf("boot ok\r\n");
  pal_logf("modbus addr: %u\r\n", (unsigned int)bsp_get_modbus_address());

  modbus_service_init();
  analog_io_service_init();

  while (1)
  {
    modbus_service_run_once();
    analog_io_service_run_once();
    bsp_heartbeat_process();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
