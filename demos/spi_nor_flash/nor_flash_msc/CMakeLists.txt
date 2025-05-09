# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CONFIG_FREERTOS 1)
set(CONFIG_SPI_NOR_FLASH 1)
# set(CONFIG_DMA_MGR 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(nor_flash_msc_demo)

sdk_inc(../common/port)
sdk_inc(../common/port/${BOARD})
sdk_inc(src)
sdk_app_src(../common/port/hpm_serial_nor_host_port.c)

if(CONFIG_SPI_NOR_FLASH EQUAL 0)
    sdk_inc($ENV{HPM_SDK_BASE}/components/serial_nor)
    sdk_app_src($ENV{HPM_SDK_BASE}/components/serial_nor/interface/spi/hpm_serial_nor_host_spi.c)
    sdk_app_src($ENV{HPM_SDK_BASE}/components/serial_nor/hpm_serial_nor.c)
endif()

sdk_app_src(src/msc_qspi_flash.c)
sdk_app_src(src/main.c)

sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)
sdk_compile_definitions(-DCONFIG_USBDEV_MSC_THREAD=1)

sdk_compile_options("-O3")

generate_ses_project()
