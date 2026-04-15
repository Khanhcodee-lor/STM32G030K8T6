#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "modbus_map.h"
#include <stdint.h>

#define APP_LED_HEARTBEAT_PERIOD_MS 500U
#define APP_DEBUG_UART_BAUDRATE     115200U
#define APP_DEBUG_RUNTIME_STATUS_PERIOD_MS 2000U
#define APP_RS485_MODBUS_BAUDRATE   9600U
#define APP_RS485_RX_BUFFER_SIZE    64U
#define APP_WATCHDOG_ENABLE         0U
#define APP_WATCHDOG_TIMEOUT_MS     1500U
#define APP_WATCHDOG_FREEZE_IN_DEBUG 1U
#define APP_I2C_SCAN_TIMEOUT_MS     2U
#define APP_I2C_SCAN_READY_TRIALS   2U
#define APP_AI_RAW_MAX              4095U
#define APP_AI_SCALED_MAX_MV        10000U
#define APP_AI_ADS1115_CONVERSION_WAIT_MS 2U
#define APP_AI_ADS1115_I2C_TIMEOUT_MS     3U
#define APP_AI_ADS1115_RETRY_DELAY_MS     20U
/* Tune these if the analog front-end maps 0..10V to a different ADS1115 full-scale point. */
#define APP_AI_ADS1115_POSITIVE_CODE_MAX  32767U
/* board raw = giá trị raw theo front-end thực tế của board tại 10V input. */
#define APP_AI_CALIBRATED_RAW_AT_10V      1931U
#define APP_AI_DEFAULT_BOARD_RAW_ZERO_AT_0V 0U
#define APP_AI_ADS1115_PGA_CONFIG         0x0200U
#define APP_AO_SETPOINT_MAX               4095U
#define APP_AO_MODE_VOLTAGE               0U
#define APP_AO_MODE_CURRENT               1U
#define APP_AO_SAFE_DEFAULT_CH1_SETPOINT  0U
#define APP_AO_SAFE_DEFAULT_CH1_MODE      APP_AO_MODE_VOLTAGE
#define APP_AO_SAFE_DEFAULT_CH2_SETPOINT  0U
#define APP_AO_SAFE_DEFAULT_CH2_MODE      APP_AO_MODE_VOLTAGE
#define APP_AO_SAFE_DEFAULT_CH3_SETPOINT  0U
#define APP_AO_SAFE_DEFAULT_CH3_MODE      APP_AO_MODE_VOLTAGE
#define APP_AO_SAFE_DEFAULT_CH4_SETPOINT  0U
#define APP_AO_SAFE_DEFAULT_CH4_MODE      APP_AO_MODE_VOLTAGE
#define APP_AO_SAFE_BOOT_APPLY_TIMEOUT_MS 100U
#define APP_AO_SAFE_BOOT_HOLD_PD_BITS     1U
#define APP_AO_APPLY_DELAY_MS             2U
#define APP_AO_APPLY_RETRY_DELAY_MS       20U
#define APP_AO_MCP4728_I2C_TIMEOUT_MS     3U
/*
 * MCP4728 per-mode defaults.
 * The DAC itself does not natively know "0..10V" vs "4..20mA"; these mode
 * values are mapped here to MCP4728 config bits so board-specific behavior can
 * be tuned without touching the driver logic again.
 */
#define APP_AO_MCP4728_UDAC_BIT                 1U
#define APP_AO_MCP4728_MODE_VOLTAGE_VREF_BIT    0U
#define APP_AO_MCP4728_MODE_VOLTAGE_PD_BITS     0U
#define APP_AO_MCP4728_MODE_VOLTAGE_GAIN_BIT    0U
#define APP_AO_MCP4728_MODE_CURRENT_VREF_BIT    0U
#define APP_AO_MCP4728_MODE_CURRENT_PD_BITS     0U
#define APP_AO_MCP4728_MODE_CURRENT_GAIN_BIT    0U
/*
 * The public AO contract is normalized at the module terminal:
 *   - Voltage mode: AO_SETPOINT 0..4095 <=> 0..10V
 *   - Current mode: AO_SETPOINT 0..4095 <=> 4..20mA
 *
 * Hardware path:
 *   MCP4728 (0..5V) -> XTR111 current transmitter (RSET = 1.5k) -> output
 *   switch -> optional 499R shunt to GND for 0..10V mode.
 *
 * Therefore firmware must convert the engineering setpoint above into the raw
 * DAC code that produces the required VIN on XTR111.
 */
#define APP_AO_MODE_VOLTAGE_DAC_MIN_CODE        0U
#define APP_AO_MODE_VOLTAGE_DAC_MAX_CODE        2462U
#define APP_AO_MODE_CURRENT_DAC_MIN_CODE        491U
#define APP_AO_MODE_CURRENT_DAC_MAX_CODE        2457U
#define APP_FW_VERSION                    0x0104U
#define APP_STATUS_MODULE_READY_BIT       (1U << 0)
#define APP_STATUS_AI_OVERRANGE_BIT       (1U << 1)
#define APP_STATUS_AO_FAULT_BIT           (1U << 2)
#define APP_ERROR_CODE_NONE               0x0000U
#define APP_ERROR_CODE_AI_ADC_FAULT       0x0001U
#define APP_ERROR_CODE_AO_DAC_FAULT       0x0002U
#define APP_ERROR_CODE_POWER_SUPPLY       0x0003U
#define APP_MODBUS_FRAME_TIMEOUT_MS 5U
#define APP_MODBUS_ADDRESS_APPLY_DELAY_MS 20U
#define APP_MODBUS_ADDRESS_MIN      1U
#define APP_MODBUS_ADDRESS_MAX      7U
#define APP_MODBUS_DEFAULT_ADDRESS  1U

#define APP_CONFIG_FLASH_PAGE_SIZE        FLASH_PAGE_SIZE
#define APP_CALIBRATION_FLASH_ADDRESS     (0x0800F000UL)
#define APP_CALIBRATION_FLASH_PAGE_INDEX  (APP_CALIBRATION_FLASH_ADDRESS / APP_CONFIG_FLASH_PAGE_SIZE - (FLASH_BASE / APP_CONFIG_FLASH_PAGE_SIZE))
#define APP_CALIBRATION_FLASH_MAGIC       0x43414C31UL
#define APP_CALIBRATION_FLASH_VERSION     0x00000001UL // unsigned long
#define APP_CONFIG_FLASH_ADDRESS          (0x0800F800UL)
#define APP_CONFIG_FLASH_PAGE_INDEX       (APP_CONFIG_FLASH_ADDRESS / APP_CONFIG_FLASH_PAGE_SIZE - (FLASH_BASE / APP_CONFIG_FLASH_PAGE_SIZE))
#define APP_CONFIG_FLASH_MAGIC            0x43464731UL
#define APP_CONFIG_FLASH_VERSION          0x00000001UL

extern const char g_app_rs485_boot_banner[];

#endif /* APP_CONFIG_H */
