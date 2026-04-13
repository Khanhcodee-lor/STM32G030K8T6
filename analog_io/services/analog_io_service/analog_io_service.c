#include "analog_io_service.h"

#include "adc.h"
#include "app_config.h"
#include "dac.h"
#include "i2c_bus_scan.h"
#include "pal.h"
#include <stddef.h>

typedef enum
{
  ANALOG_IO_SERVICE_STATE_IDLE = 0,
  ANALOG_IO_SERVICE_STATE_SCANNING,
  ANALOG_IO_SERVICE_STATE_READY
} AnalogIoServiceState;

static AnalogIoServiceState s_state;
static I2cBusScanReport s_i2c_scan_report;
static uint8_t s_scan_address;
static uint8_t s_ai_initialized;
static uint8_t s_ao_initialized;
static uint8_t s_is_initialized;

static void AnalogIoService_LogI2cScanReport(const I2cBusScanReport *report);
static void AnalogIoService_CompleteBringUp(void);

int analog_io_service_apply_safe_output_blocking(void)
{
  uint32_t start_tick_ms = pal_millis();
  int is_ok = 0;

  dac_reset_to_safe_defaults();
  is_ok = dac_apply_safe_state_blocking(APP_AO_SAFE_BOOT_APPLY_TIMEOUT_MS) ? 1 : 0;

  pal_logf("ao safe state %s in %lu ms\r\n",
           (is_ok != 0) ? "applied" : "failed",
           (unsigned long)(pal_millis() - start_tick_ms));
  return is_ok;
}

void analog_io_service_init(void)
{
  I2cBusScan_ResetReport(&s_i2c_scan_report);
  s_scan_address = I2C_BUS_SCAN_ADDR_MIN;
  s_ai_initialized = 0U;
  s_ao_initialized = 0U;
  s_state = ANALOG_IO_SERVICE_STATE_SCANNING;
  s_is_initialized = 1U;

  pal_logf("i2c2 staged scan start: 0x%02X..0x%02X\r\n",
           I2C_BUS_SCAN_ADDR_MIN,
           I2C_BUS_SCAN_ADDR_MAX);
}

void analog_io_service_run_once(void)
{
  if (s_is_initialized == 0U)
  {
    return;
  }

  if (s_state == ANALOG_IO_SERVICE_STATE_SCANNING)
  {
    I2cBusScan_ScanAddress(&s_i2c_scan_report, s_scan_address);

    if ((s_ai_initialized == 0U) && (s_i2c_scan_report.ads1115_found != 0U))
    {
      adc_init(s_i2c_scan_report.ads1115_address);
      s_ai_initialized = 1U;
      pal_logf("ai ads1115 ready @ 0x%02X\r\n", s_i2c_scan_report.ads1115_address);
    }

    if ((s_ao_initialized == 0U) && (s_i2c_scan_report.mcp4728_found != 0U))
    {
      dac_init(s_i2c_scan_report.mcp4728_address);
      s_ao_initialized = 1U;
      pal_logf("ao mcp4728 ready @ 0x%02X\r\n", s_i2c_scan_report.mcp4728_address);
    }

    if (s_scan_address >= I2C_BUS_SCAN_ADDR_MAX)
    {
      AnalogIoService_CompleteBringUp();
    }
    else
    {
      ++s_scan_address;
    }
  }

  if (s_ai_initialized != 0U)
  {
    adc_process();
  }

  dac_process();
}

static void AnalogIoService_CompleteBringUp(void)
{
  AnalogIoService_LogI2cScanReport(&s_i2c_scan_report);

  if (s_ai_initialized == 0U)
  {
    adc_init(0U);
    s_ai_initialized = 1U;
    pal_logf("ai ads1115: not found\r\n");
  }

  if (s_ao_initialized == 0U)
  {
    dac_init(0U);
    s_ao_initialized = 1U;
    pal_logf("ao mcp4728: not found\r\n");
  }

  s_state = ANALOG_IO_SERVICE_STATE_READY;
}

static void AnalogIoService_LogI2cScanReport(const I2cBusScanReport *report)
{
  uint8_t i;
  uint8_t listed_count;

  if (report == NULL)
  {
    return;
  }

  listed_count = report->found_count;
  if (listed_count > I2C_BUS_SCAN_MAX_FOUND_DEVICES)
  {
    listed_count = I2C_BUS_SCAN_MAX_FOUND_DEVICES;
  }

  if (report->found_count == 0U)
  {
    pal_logf("i2c2 scan: no ack\r\n");
  }
  else
  {
    pal_logf("i2c2 devices: %u\r\n", report->found_count);
    for (i = 0U; i < listed_count; ++i)
    {
      pal_logf("i2c2 ack @ 0x%02X\r\n", report->found_addresses[i]);
    }

    if (report->overflow != 0U)
    {
      pal_logf("i2c2 scan: result truncated after %u addresses\r\n", I2C_BUS_SCAN_MAX_FOUND_DEVICES);
    }
  }

  if (report->ads1115_found != 0U)
  {
    pal_logf("ads1115 ack @ 0x%02X\r\n", report->ads1115_address);
  }
  else
  {
    pal_logf("ads1115 ack: not found in 0x48..0x4B\r\n");
  }

  if (report->mcp4728_found != 0U)
  {
    pal_logf("mcp4728 ack @ 0x%02X\r\n", report->mcp4728_address);
  }
  else
  {
    pal_logf("mcp4728 ack: not found in 0x60..0x67\r\n");
  }
}
