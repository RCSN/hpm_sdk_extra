# Copyright (c) 2024 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(i2c_master_rtx_dma_chain_demo)

sdk_app_inc(inc)
sdk_app_src(src/hpm_i2c_dma_chain.c)
sdk_app_src(src/i2c.c)
generate_ide_projects()
