#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "modbus_map.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint16_t ai_board_raw_zero[MODBUS_MAP_AI_CHANNEL_COUNT];
  uint16_t ai_board_raw_full_scale[MODBUS_MAP_AI_CHANNEL_COUNT];
  uint16_t ao_voltage_min_code[MODBUS_MAP_AO_CHANNEL_COUNT];
  uint16_t ao_voltage_max_code[MODBUS_MAP_AO_CHANNEL_COUNT];
  uint16_t ao_current_min_code[MODBUS_MAP_AO_CHANNEL_COUNT];
  uint16_t ao_current_max_code[MODBUS_MAP_AO_CHANNEL_COUNT];
} CalibrationData;

void Calibration_Init(void);
bool Calibration_IsFactoryDataLoaded(void);
const CalibrationData *Calibration_Get(void);
void Calibration_LoadDefaults(CalibrationData *data);
bool Calibration_Save(const CalibrationData *data);

#endif /* CALIBRATION_H */
