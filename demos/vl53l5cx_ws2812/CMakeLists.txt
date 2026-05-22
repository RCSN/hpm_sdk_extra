# Copyright (c) 2026 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

if("${HPM_BUILD_TYPE}" STREQUAL "")
    SET(HPM_BUILD_TYPE flash_xip)
endif()

# Enable SPI/I2C components and DMA manager
set(CONFIG_HPM_SPI 1)
set(CONFIG_HPM_I2C 1)
set(CONFIG_DMA_MGR 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(vl53l5cx_ws2812)

sdk_ld_options("-lm")

# ============================================================================
# Board pin configuration (via compile definitions)
# All I2C/SPI/GPIO pins are defined here, drivers self-initialize pads.
# This decouples the application from board.c pinmux.
# ============================================================================

# WS2812 SPI1 MOSI pin
sdk_compile_definitions(-DWS2812_DIN=IOC_PAD_PA29)

# VL53L5CX Sensor A I2C2 pins
sdk_compile_definitions(-DVL53L5CX_A_I2C_SCL=IOC_PAD_PB08)
sdk_compile_definitions(-DVL53L5CX_A_I2C_SDA=IOC_PAD_PB09)

# VL53L5CX Sensor B I2C3 pins
sdk_compile_definitions(-DVL53L5CX_B_I2C_SCL=IOC_PAD_PB13)
sdk_compile_definitions(-DVL53L5CX_B_I2C_SDA=IOC_PAD_PB12)

# VL53L5CX Sensor A SATEL control pins (GPIO)
sdk_compile_definitions(-DVL53L5CX_A_PWR_EN=IOC_PAD_PA09)
sdk_compile_definitions(-DVL53L5CX_A_LPN=IOC_PAD_PB10)
sdk_compile_definitions(-DVL53L5CX_A_I2C_RST=IOC_PAD_PB11)

# VL53L5CX Sensor B SATEL control pins (GPIO)
sdk_compile_definitions(-DVL53L5CX_B_PWR_EN=IOC_PAD_PA31)
sdk_compile_definitions(-DVL53L5CX_B_LPN=IOC_PAD_PB14)
sdk_compile_definitions(-DVL53L5CX_B_I2C_RST=IOC_PAD_PB15)

# VL53L5CX driver sources (use sdk_app_src for full include paths)
sdk_inc(drivers/vl53l5cx)
sdk_app_src(drivers/vl53l5cx/vl53l5cx_api.c)
sdk_app_src(drivers/vl53l5cx/vl53l5cx_platform_hpm.c)
sdk_app_src(drivers/vl53l5cx/vl53l5cx_plugin_motion_indicator.c)

# WS2812 driver sources
sdk_inc(drivers/ws2812)
sdk_app_src(drivers/ws2812/ws2812.c)

# Application sources
sdk_inc(src)
sdk_app_src(src/main.c)
sdk_app_src(src/gesture_engine.c)
sdk_app_src(src/app_depth_heatmap.c)
sdk_app_src(src/app_gesture_light.c)
sdk_app_src(src/app_ripple.c)
sdk_app_src(src/app_dual_tof.c)

generate_ide_projects()
