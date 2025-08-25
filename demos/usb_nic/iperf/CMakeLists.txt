# Copyright (c) 2021-2024 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_FREERTOS 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_CHERRYUSB_HOST 1)
set(CONFIG_USB_HOST_RNDIS 1)
set(CONFIG_USB_HOST_CDC_ECM 1)
set(CONFIG_LWIP 1)
set(CONFIG_LWIP_IPERF 1)
set(CONFIG_LWIP_STRERR 1)
set(CONFIG_LWIP_NETDB 1)
set(CONFIG_LWIP_SOCKET_API 1)
set(SES_COMPILER_VARIANT "SEGGER")
if("${HPM_BUILD_TYPE}" STREQUAL "")
    SET(HPM_BUILD_TYPE flash_xip)
endif()

# Select the customized linker file
if (${BOARD} STREQUAL "hpm6200evk")
    set(CUSTOM_GCC_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linker_file/HPM6200/gcc/usb_nic_flash_xip.ld)
    set(CUSTOM_SES_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linker_file/HPM6200/segger/usb_nic_flash_xip.icf)
elseif (${BOARD} STREQUAL "hpm5e00evk")
    set(CUSTOM_GCC_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linker_file/HPM5E00/gcc/usb_nic_flash_xip.ld)
    set(CUSTOM_SES_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/linker_file/HPM5E00/segger/usb_nic_flash_xip.icf)
endif ()

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherryusb_host_usbnic_iperf)

sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)
sdk_compile_definitions(-DCONFIG_IPERF_TCP_SERVER_PORT=5001)    
sdk_compile_definitions(-DconfigTOTAL_HEAP_SIZE=36864)
sdk_compile_definitions(-DLWIP_TIMEVAL_PRIVATE=0)
sdk_compile_definitions(-DCONFIG_USBHOST_PLATFORM_ASIX=1)
sdk_compile_definitions(-DCONFIG_USBHOST_PLATFORM_RTL8152=1)

sdk_iar_cc_preinclude(../../../../../drivers/inc/hpm_common.h)

sdk_inc(../inc)
sdk_inc(src)
sdk_inc(../common)
sdk_inc($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/vendor/net)

sdk_app_src($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/vendor/net/usbh_asix.c)
sdk_app_src($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/vendor/net/usbh_rtl8152.c)

sdk_app_src(../common/arch/sys_arch.c)
sdk_app_src(src/main.c)
sdk_app_src(src/iperf.c)
generate_ide_projects()
