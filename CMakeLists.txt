cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(STM32G030K8 LANGUAGES C ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Top-level build delegates to analog_io/")

add_subdirectory(analog_io)
