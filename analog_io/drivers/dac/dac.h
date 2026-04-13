#ifndef DAC_H
#define DAC_H

#include <stdbool.h>
#include <stdint.h>

void dac_init(uint8_t address);
void dac_process(void);
bool dac_read_holding_register(uint16_t address, uint16_t *value);
bool dac_write_holding_register(uint16_t address, uint16_t value);
bool dac_write_holding_registers(uint16_t start_address, const uint16_t *values, uint16_t count);
uint16_t dac_get_status(void);
uint16_t dac_get_error_code(void);

#endif /* DAC_H */
