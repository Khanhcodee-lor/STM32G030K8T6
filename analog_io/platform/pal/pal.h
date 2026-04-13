#ifndef PAL_H
#define PAL_H

#include <stdint.h>

void pal_debug_init(void);
void pal_logf(const char *fmt, ...);
uint32_t pal_millis(void);
void pal_delay_ms(uint32_t delay_ms);
void pal_led_set(uint8_t state);
void pal_led_toggle(void);

#endif /* PAL_H */
