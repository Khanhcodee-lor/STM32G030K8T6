#ifndef CONFIG_H
#define CONFIG_H

#include "app_config.h"

#define MODBUS_BAUD_RATE              APP_RS485_MODBUS_BAUDRATE
#define MODBUS_ADDR_MIN               APP_MODBUS_ADDRESS_MIN
#define MODBUS_ADDR_MAX               APP_MODBUS_ADDRESS_MAX
#define MODBUS_ADDR_REGISTER          APP_DEVICE_REG_DEVICE_ID
#define MODBUS_FRAME_GAP_MS           APP_MODBUS_FRAME_TIMEOUT_MS
#define DEBUG_UART_BAUD_RATE          APP_DEBUG_UART_BAUDRATE
#define ANALOG_SCAN_PERIOD_MS         APP_AI_ADS1115_CONVERSION_WAIT_MS
#define AI_CHANNELS                   APP_AI_CHANNEL_COUNT
#define AO_CHANNELS                   APP_AO_CHANNEL_COUNT
#define ADC_RESOLUTION                12U
#define DAC_RESOLUTION                12U
#define RS485_LOW_LEVEL_TEST          1

#endif /* CONFIG_H */
