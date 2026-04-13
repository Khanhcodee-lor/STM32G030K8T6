#ifndef BSP_H
#define BSP_H

#include <stdint.h>

void bsp_init(void);
void bsp_heartbeat_process(void);
uint8_t bsp_get_modbus_address(void);
int bsp_set_modbus_address(uint8_t address);

#endif /* BSP_H */
