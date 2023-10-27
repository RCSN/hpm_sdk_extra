# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)
# set(CONFIG_SPI_NOR_FLASH 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(spi_nor_flash_demo)

sdk_inc($ENV{HPM_SDK_BASE}/components/serial_nor)
sdk_inc(../common/port)
sdk_inc(../common/port/${BOARD})

sdk_app_src($ENV{HPM_SDK_BASE}/components/serial_nor/interface/spi/hpm_serial_nor_host_spi.c)
sdk_app_src($ENV{HPM_SDK_BASE}/components/serial_nor/hpm_serial_nor.c)
sdk_app_src(../common/port/hpm_serial_nor_host_port.c)
sdk_app_src(src/main.c)

sdk_compile_options("-O3")

generate_ses_project()
