# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC 1)
set(CONFIG_HPM_PANEL 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(hpm_usb_display)

sdk_inc(../../config)
sdk_inc(graphic)
sdk_app_src(graphic/usbd_graphic.c)

sdk_app_src(src/usb_display.c)
sdk_app_src(src/main.c)
generate_ses_project()
