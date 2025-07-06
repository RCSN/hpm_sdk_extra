# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

# Copyright (c) 2021-2025  HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)
# set(APP_NAME "lvgl_spi_lcd")

# set LVGL MAJOR VERSION
set(ENV{LVGL_MAJOR_VERSION} 9)
# set(ENV{LVGL_MAJOR_VERSION} 8)

# set heap and stack
set(HEAP_SIZE 0x8000)
set(STACK_SIZE 0x10000)

# set use te sync
set(ENV{LVGL_USE_TE_SYNC} 1)

# lvgl buffer set
set(ENV{LVGL_USE_FULL_BUFFER} 1)

set(CONFIG_HPM_SPI 1)
set(CONFIG_DMA_MGR 1)
set(CONFIG_LV_DEMO 1)

set(CONFIG_FREERTOS 1)

# ENABLE USB HID FOR CHERRYUSB
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_HOST_HID 1)
set(CONFIG_USB_HOST_HID_LVGL 1)

# set(CONFIG_LV_DEMO_NAME music)
# set(CONFIG_LV_DEMO_NAME stress)
# set(CONFIG_LV_DEMO_NAME widgets)
set(CONFIG_LV_DEMO_NAME benchmark)

if ($ENV{LVGL_MAJOR_VERSION} EQUAL 9)
    set(CONFIG_LVGL 1)
    set(CONFIG_LVGL_CUSTOM_PORTABLE 1)
elseif ($ENV{LVGL_MAJOR_VERSION} EQUAL 8)
endif()

if(CONFIG_FREERTOS EQUAL 1)
    set(CONFIG_FREERTOS_TIMER_RESOURCE_NOT_MTIMER 1)
endif()

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(lvgl_spi_lcd)

if ($ENV{LVGL_USE_FULL_BUFFER} EQUAL 1)
    sdk_compile_definitions(-DLVGL_USE_FULL_BUFFER=1)
    # sdk_compile_definitions(-DLVGL_USE_DIRECT_MODE=1)
else()
    sdk_compile_definitions(-DLVGL_USE_FULL_BUFFER=0)
endif()


if (CONFIG_FREERTOS EQUAL 1)
    sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
    sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=0)
    sdk_compile_definitions(-DUSE_FREERTOS_OS=1)
endif()

# sdk_compile_definitions(-DENABLE_NV3007_LCD_DRIVER=1)
sdk_compile_definitions(-DENABLE_ST7789_LCD_DRIVER=1)

if (CONFIG_LV_DEMO_NAME MATCHES "stress")
    sdk_compile_definitions(-DLV_USE_DEMO_STRESS=1)
elseif (CONFIG_LV_DEMO_NAME MATCHES "widgets")
    sdk_compile_definitions(-DLV_USE_DEMO_WIDGETS=1)
elseif (CONFIG_LV_DEMO_NAME MATCHES "benchmark")
    sdk_compile_definitions(-DLV_USE_DEMO_BENCHMARK=1)
elseif (CONFIG_LV_DEMO_NAME MATCHES "music")
    sdk_compile_definitions(-DLV_USE_DEMO_MUSIC=1)
endif()

if ($ENV{LVGL_MAJOR_VERSION} EQUAL 9)
# sdk_compile_definitions(-DLV_LOG_LEVEL=LV_LOG_LEVEL_INFO)
# sdk_compile_definitions(-DLV_USE_SYSMON=1)
    sdk_compile_definitions(-DLV_COLOR_16_SWAP=0)
    sdk_compile_definitions(-DLVGL_MAJOR_VERSION=9)
    if (CONFIG_LV_DEMO_NAME MATCHES "benchmark")
        sdk_compile_definitions(-DLV_USE_DEMO_WIDGETS=1)
    endif()
elseif ($ENV{LVGL_MAJOR_VERSION} EQUAL 8)
    sdk_compile_definitions(-DLVGL_MAJOR_VERSION=8)
    sdk_inc(lvgl_v835/lvgl)
    sdk_app_src_glob(lvgl_v835/lvgl/src/*.c)
    if(DEFINED CONFIG_LV_DEMO_NAME)
        file(GLOB_RECURSE LVGL_DEMO_SRC lvgl_v835/lvgl/demos/${CONFIG_LV_DEMO_NAME}/*.c)
        sdk_src(${LVGL_DEMO_SRC})
    endif()
endif()

sdk_compile_definitions(-DCONFIG_LV_HAS_EXTRA_CONFIG="lv_app_conf.h")
sdk_compile_definitions(-DLV_DRAW_BUF_STRIDE_ALIGN=1)
sdk_compile_definitions(-DUSE_HORIZONTIAL=1)
sdk_compile_definitions(-DSPI_TFT_LCD_DC_PIN=IOC_PAD_PF25)
sdk_compile_definitions(-DSPI_TFT_LCD_RST_PIN=IOC_PAD_PF09)
sdk_compile_definitions(-DSPI_LVGL_TEST_TX_PIN=IOC_PAD_PF02)
sdk_compile_definitions(-DSPI_LVGL_REFR_TIMER_PIN=IOC_PAD_PF15)

if ($ENV{LVGL_USE_TE_SYNC} EQUAL 1)
    sdk_compile_definitions(-DSPI_TFT_LCD_TE_PIN=IOC_PAD_PF28)
    sdk_compile_definitions(-DSPI_TFT_LCD_TE_IRQ=IRQn_GPIO0_F)
    sdk_compile_definitions(-DLVGL_USE_TE_SYNC=1)
    sdk_compile_definitions(-DLVGL_USE_TE_REFR=0)
endif()
sdk_app_src_glob(src/*.c)

sdk_inc(inc)

# sdk_compile_options("-O3")

# sdk_ses_opt_debug_connection(J-Link)
generate_ide_projects()

