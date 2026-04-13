#include "bsp.h"
#include "config.h"
#include "main.h"
#include "modbus_service.h"
#include "analog_io_service.h"
#include "pal.h"

int main(void)
{
  uint32_t modbus_ready_ms = 0U;
  uint32_t boot_total_ms = 0U;

  HAL_Init();
  bsp_init();
  bsp_watchdog_init();

  pal_logf("reset t=0ms cause=%s\r\n", bsp_get_reset_cause_name());
  pal_logf("modbus addr: %u\r\n", (unsigned int)bsp_get_modbus_address());
  (void)analog_io_service_apply_safe_output_blocking();

  modbus_service_init();
  analog_io_service_init();
  modbus_ready_ms = pal_millis();
  boot_total_ms = pal_millis();

  pal_logf("modbus ready time: %lu ms\r\n", (unsigned long)modbus_ready_ms);
  pal_logf("boot total time: %lu ms\r\n", (unsigned long)boot_total_ms);

  while (1)
  {
    modbus_service_run_once();
    analog_io_service_run_once();
    bsp_heartbeat_process();
    bsp_watchdog_kick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
