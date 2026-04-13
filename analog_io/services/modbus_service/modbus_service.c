#include "modbus_service.h"

#include "bsp.h"
#include "config.h"
#include "modbus_rtu.h"
#include "pal.h"
#include "rs485.h"

static uint8_t s_is_initialized;

void modbus_service_init(void)
{
  rs485_init(MODBUS_BAUD_RATE);
  ModbusRtu_Init();
  pal_logf("rs485/modbus ready @ %lu\r\n", (unsigned long)MODBUS_BAUD_RATE);
  s_is_initialized = 1U;
}

void modbus_service_run_once(void)
{
  if (s_is_initialized == 0U)
  {
    return;
  }

  ModbusRtu_Poll(bsp_get_modbus_address());
}
