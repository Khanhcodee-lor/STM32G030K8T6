#include "bsp.h"
#include "calibration.h"
#include "calibration_test_hook.h"
#include "config.h"
#include "main.h"
#include "modbus_rtu.h"
#include "modbus_service.h"
#include "analog_io_service.h"
#include "pal.h"

static void Main_LogRuntimeStatusIfDue(void);

int main(void)
{
  uint32_t modbus_ready_ms = 0U;
  uint32_t boot_total_ms = 0U;
  uint8_t calibration_test_sample_written = 0U;

  HAL_Init();
  bsp_init();
  bsp_watchdog_init();
#if (APP_CALIBRATION_TEST_HOOK_ENABLE != 0U)
  calibration_test_sample_written = CalibrationTestHook_InstallSampleIfNeeded() ? 1U : 0U;
#endif

  pal_logf("reset t=0ms cause=%s\r\n", bsp_get_reset_cause_name());
  pal_logf("modbus addr: %u\r\n", (unsigned int)bsp_get_modbus_address());
  if (calibration_test_sample_written != 0U)
  {
    pal_logf("calibration test hook: sample saved\r\n");
  }
  pal_logf("calibration: %s\r\n", Calibration_IsFactoryDataLoaded() ? "flash" : "defaults");
  (void)analog_io_service_apply_safe_output_blocking();

  modbus_service_init();
  analog_io_service_init();
  modbus_ready_ms = pal_millis();
  boot_total_ms = pal_millis();

  pal_logf("modbus ready time: %lu ms\r\n", (unsigned long)modbus_ready_ms);
  pal_logf("boot total time: %lu ms\r\n", (unsigned long)boot_total_ms);

  while (1)
  {
    modbus_service_run_once();
    analog_io_service_run_once();
    Main_LogRuntimeStatusIfDue();
    bsp_heartbeat_process();
    bsp_watchdog_kick();
  }
}

static void Main_LogRuntimeStatusIfDue(void)
{
#if (APP_DEBUG_RUNTIME_STATUS_PERIOD_MS != 0U)
  static uint32_t s_next_log_tick_ms = APP_DEBUG_RUNTIME_STATUS_PERIOD_MS;
  uint32_t now_ms = pal_millis();

  if (now_ms < s_next_log_tick_ms)
  {
    return;
  }

  pal_logf("runtime t=%lu ms calib=%s status=0x%04X error=0x%04X addr=%u\r\n",
           (unsigned long)now_ms,
           Calibration_IsFactoryDataLoaded() ? "flash" : "defaults",
           (unsigned int)ModbusRtu_GetStatusRegister(),
           (unsigned int)ModbusRtu_GetErrorCode(),
           (unsigned int)bsp_get_modbus_address());

  s_next_log_tick_ms = now_ms + APP_DEBUG_RUNTIME_STATUS_PERIOD_MS;
#endif
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
