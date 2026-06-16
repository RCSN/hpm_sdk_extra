# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYRB 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC 1)
set(CONFIG_DMA_MGR 1)
set(CONFIG_HPM_SPI 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(hpmicro_cangaroo)

# sdk_compile_definitions(-DSLCAN_DEBUG=printf)
sdk_compile_definitions(-DWS2812_USE_SPI=1)
sdk_compile_definitions(-DCONFIG_SLCAN0)
sdk_compile_definitions(-DCONFIG_SLCAN1)
sdk_compile_definitions(-DCONFIG_SLCAN2)
sdk_compile_definitions(-DCONFIG_SLCAN3)

sdk_inc(inc)
sdk_inc(protocol/slcan/inc)
sdk_inc(ws2812)
sdk_inc(protocol)
sdk_inc(protocol/driver/mcan)
sdk_app_src(protocol/driver/mcan/mcan.c)
sdk_app_src(protocol/slcan/src/slcan.c)
sdk_app_src(protocol/usb2can.c)
sdk_app_src(src/main.c)
sdk_app_src(src/cdc_acm.c)
sdk_app_src(ws2812/WS2812.c)


generate_ide_projects()
