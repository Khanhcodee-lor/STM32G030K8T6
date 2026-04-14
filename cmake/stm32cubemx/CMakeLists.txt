cmake_minimum_required(VERSION 3.22)

# Compatibility stub.
# The active firmware build moved to analog_io/, so the old STM32CubeMX graph
# is intentionally no longer maintained here.

if(NOT TARGET stm32cubemx)
    add_library(stm32cubemx INTERFACE)
endif()

message(STATUS "Deprecated CMake path: cmake/stm32cubemx is kept only for compatibility")
