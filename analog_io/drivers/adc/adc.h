#ifndef ADC_H
#define ADC_H

#include <stdbool.h>
#include <stdint.h>

void adc_init(uint8_t address);
void adc_process(void);
bool adc_read_input_register(uint16_t address, uint16_t *value);
uint16_t adc_get_status(void);
uint16_t adc_get_error_code(void);

#endif /* ADC_H */
