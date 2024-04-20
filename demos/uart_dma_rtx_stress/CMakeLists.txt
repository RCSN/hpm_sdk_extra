# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_DMA_MGR 1)
set(CONFIG_CHERRYRB 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(uart_dma_rtx_stress)

sdk_app_src(src/uart_dma_rtx_stress.c)
generate_ide_projects()
