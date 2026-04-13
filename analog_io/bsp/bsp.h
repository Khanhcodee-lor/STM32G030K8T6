#ifndef BSP_H
#define BSP_H

#include <stdint.h>

typedef enum
{
  BSP_RESET_CAUSE_UNKNOWN = 0,
  BSP_RESET_CAUSE_POWER,
  BSP_RESET_CAUSE_PIN,
  BSP_RESET_CAUSE_SOFTWARE,
  BSP_RESET_CAUSE_IWDG,
  BSP_RESET_CAUSE_WWDG,
  BSP_RESET_CAUSE_LOW_POWER
} BspResetCause;

void bsp_init(void);
void bsp_heartbeat_process(void);
uint8_t bsp_get_modbus_address(void);
int bsp_set_modbus_address(uint8_t address);
BspResetCause bsp_get_reset_cause(void);
const char *bsp_get_reset_cause_name(void);
void bsp_watchdog_init(void);
void bsp_watchdog_kick(void);

#endif /* BSP_H */
