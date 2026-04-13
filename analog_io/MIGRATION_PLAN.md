# Analog IO Migration Plan (Base Cong Ty)

## Hien trang
- Cau truc thu muc da tao dung huong base cong ty:
  - app/
  - bsp/
  - components/
  - config/
  - drivers/
  - platform/
  - services/
- Base HAL moi dang chay theo flow:
  - app/main.c
  - bsp/bsp.c
  - services/modbus_service/
  - services/i2c_bus_scan/
  - services/analog_io_service/
  - drivers/adc, drivers/dac, drivers/rs485
  - platform/pal, platform/mcu
  - config/app_config.*, config/device_config.*, config/config.h

## Muc tieu layout
- app/: entry point va startup flow
- bsp/: init board va doc DIP switch 1 lan luc boot
- services/: service/task level (analog io, modbus)
- components/: logic thuong mai, parser, scaling
- drivers/: adc/dac/rs485 driver
- platform/: pal/osal/mcu startup + irq + linker
- config/: constants, register map, timeout, pin map

## Mapping tu base cu (Core/*)
| Base cu | Dich sang analog_io | Ghi chu |
|---|---|---|
| Core/Src/main.c | app/main.c | Entry point moi |
| Core/app/app_main.c | services/analog_io_service + services/modbus_service | Tach polling thanh service |
| Core/app/analog_input.* | components/analog_core + drivers/adc | Tach logic va hw access |
| Core/app/analog_output.* | components/analog_core + drivers/dac | Tach logic va hw access |
| Core/protocols/modbus_rtu.* | components/modbus_rtu | Parser/state machine |
| Core/protocols/rs485_ll.* | drivers/rs485 + platform/pal | UART + DE/RE |
| Core/protocols/i2c_bus_scan.* | drivers hoac diagnostics service | Bring-up utility |
| Core/Src/stm32g0xx_it.c | platform/mcu/stm32g0xx/stm32g0xx_it.c | IRQ vector handlers |
| Core/Src/system_stm32g0xx.c | platform/mcu/stm32g0xx/system_stm32g0xx.c | Clock/system |
| startup_stm32g030xx.s | platform/mcu/stm32g0xx/startup_stm32g030xx.s | Startup asm |
| STM32G030XX_FLASH.ld | platform/mcu/stm32g0xx/STM32G030XX_FLASH.ld | Linker script |

## Thu tu migrate de giam rui ro
1. Da tao CMake target rieng cho analog_io va build/debug tu analog_io/build.
2. Da tach main moi sang app/main.c.
3. Da tach bootstrap sang bsp/bsp.c.
4. Da tach flow polling sang services/modbus_service va services/analog_io_service.
5. Da dua startup/irq/system/linker vao platform/mcu.
6. Da them driver wrapper cho adc/dac/rs485.
7. Con lai: neu muon sach hon nua thi xoa hoan toan root flow cu va cap nhat tai lieu/CI.

## Khac biet quan trong voi yeu cau cong ty
- Hien tai code dang dung HAL.
- Yeu cau cong ty: khong HAL, uu tien register-level qua platform/pal.
- Hien tai Device ID dang cho doi qua Modbus va luu Flash.
- Yeu cau cong ty: doc DIP switch 1 lan luc boot (khong doi runtime).
