# Copyright (c) 2024 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(interval_timer_demo)
sdk_inc(inc)
sdk_app_src(src/interval_timer.c)
sdk_app_src(src/main.c)
generate_ide_projects()
