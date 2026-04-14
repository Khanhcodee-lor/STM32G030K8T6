#include "calibration_test_hook.h"

#include "calibration.h"

#define CALIBRATION_TEST_AI_FULL_SCALE_RAW    1800U
#define CALIBRATION_TEST_AO_VOLTAGE_MAX_CODE  2300U
#define CALIBRATION_TEST_AO_CURRENT_MIN_CODE  520U
#define CALIBRATION_TEST_AO_CURRENT_MAX_CODE  2400U

bool CalibrationTestHook_InstallSampleIfNeeded(void)
{
#if (APP_CALIBRATION_TEST_HOOK_ENABLE != 0U)
  CalibrationData sample;
  uint8_t index = 0U;

  if (Calibration_IsFactoryDataLoaded())
  {
    return false;
  }

  Calibration_LoadDefaults(&sample);

  for (index = 0U; index < MODBUS_MAP_AI_CHANNEL_COUNT; ++index)
  {
    sample.ai_board_raw_full_scale[index] = CALIBRATION_TEST_AI_FULL_SCALE_RAW;
  }

  for (index = 0U; index < MODBUS_MAP_AO_CHANNEL_COUNT; ++index)
  {
    sample.ao_voltage_max_code[index] = CALIBRATION_TEST_AO_VOLTAGE_MAX_CODE;
    sample.ao_current_min_code[index] = CALIBRATION_TEST_AO_CURRENT_MIN_CODE;
    sample.ao_current_max_code[index] = CALIBRATION_TEST_AO_CURRENT_MAX_CODE;
  }

  if (!Calibration_Save(&sample))
  {
    return false;
  }

  Calibration_Init();
  return true;
#else
  return false;
#endif
}
