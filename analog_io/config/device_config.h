#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

void DeviceConfig_Init(void);
uint8_t DeviceConfig_GetModbusAddress(void);
bool DeviceConfig_SetModbusAddress(uint8_t address);
bool DeviceConfig_Save(void);

#endif /* DEVICE_CONFIG_H */
