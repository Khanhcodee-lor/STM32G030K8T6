#ifndef I2C_BUS_SCAN_H
#define I2C_BUS_SCAN_H

#include <stdint.h>

#define I2C_BUS_SCAN_MAX_FOUND_DEVICES 16U

typedef struct
{
  uint8_t found_addresses[I2C_BUS_SCAN_MAX_FOUND_DEVICES];
  uint8_t found_count;
  uint8_t overflow;
  uint8_t ads1115_found;
  uint8_t ads1115_address;
  uint8_t mcp4728_found;
  uint8_t mcp4728_address;
} I2cBusScanReport;

void I2cBusScan_Run(I2cBusScanReport *report);

#endif /* I2C_BUS_SCAN_H */
