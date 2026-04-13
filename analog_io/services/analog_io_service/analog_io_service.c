#include "analog_io_service.h"

#include "adc.h"
#include "dac.h"
#include "i2c_bus_scan.h"
#include "pal.h"
#include <stddef.h>

static void AnalogIoService_LogI2cScanReport(const I2cBusScanReport *report);

void analog_io_service_init(void)
{
  I2cBusScanReport i2c_scan_report;

  pal_logf("i2c2 scan range: 0x08..0x77\r\n");
  I2cBusScan_Run(&i2c_scan_report);
  AnalogIoService_LogI2cScanReport(&i2c_scan_report);

  adc_init((i2c_scan_report.ads1115_found != 0U) ? i2c_scan_report.ads1115_address : 0U);
  dac_init((i2c_scan_report.mcp4728_found != 0U) ? i2c_scan_report.mcp4728_address : 0U);

  if (i2c_scan_report.ads1115_found != 0U)
  {
    pal_logf("ai ads1115 ready @ 0x%02X\r\n", i2c_scan_report.ads1115_address);
  }
  else
  {
    pal_logf("ai ads1115: not found\r\n");
  }

  if (i2c_scan_report.mcp4728_found != 0U)
  {
    pal_logf("ao mcp4728 ready @ 0x%02X\r\n", i2c_scan_report.mcp4728_address);
  }
  else
  {
    pal_logf("ao mcp4728: not found\r\n");
  }
}

void analog_io_service_run_once(void)
{
  adc_process();
  dac_process();
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
