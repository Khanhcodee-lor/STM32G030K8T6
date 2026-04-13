cmake_minimum_required(VERSION 3.20)

project(analog_io C ASM)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/platform/mcu/STM32G030XX_FLASH.ld)

add_executable(${PROJECT_NAME}.elf
    app/main.c
    bsp/bsp.c
    services/analog_io_service/analog_io_service.c
    services/modbus_service/modbus_service.c
    services/i2c_bus_scan/i2c_bus_scan.c
    drivers/adc/adc.c
    drivers/dac/dac.c
    drivers/rs485/rs485.c
    components/analog_core/analog_input.c
    components/analog_core/analog_output.c
    components/modbus_rtu/modbus_rtu.c
    config/app_config.c
    config/device_config.c
    platform/pal/debug_log.c
    platform/pal/pal.c
    platform/pal/rs485_ll.c
    platform/mcu/system_stm32g0xx.c
    platform/mcu/stm32g0xx_it.c
    platform/mcu/startup_stm32g030xx.s
    ${PROJECT_ROOT}/Core/Src/stm32g0xx_hal_msp.c
    ${PROJECT_ROOT}/Core/Src/syscalls.c
    ${PROJECT_ROOT}/Core/Src/sysmem.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c_ex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/adc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/dac
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rs485
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/services/analog_io_service
    ${CMAKE_CURRENT_SOURCE_DIR}/services/i2c_bus_scan
    ${CMAKE_CURRENT_SOURCE_DIR}/services/modbus_service
    ${CMAKE_CURRENT_SOURCE_DIR}/components/analog_core
    ${CMAKE_CURRENT_SOURCE_DIR}/components/modbus_rtu
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/pal
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/mcu
    ${PROJECT_ROOT}/Core/Inc
    ${PROJECT_ROOT}/Drivers/CMSIS/Include
    ${PROJECT_ROOT}/Drivers/CMSIS/Device/ST/STM32G0xx/Include
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Inc
    ${PROJECT_ROOT}/Drivers/STM32G0xx_HAL_Driver/Inc/Legacy
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    USE_HAL_DRIVER
    STM32G030xx
    $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Release>:-DNDEBUG>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE m)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMENT "Converting ${PROJECT_NAME}.elf to HEX and BIN"
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Binary size information"
)
