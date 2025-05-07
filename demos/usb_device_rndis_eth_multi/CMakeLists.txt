# Copyright (c) 2024-2025 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_FREERTOS 1)

set(CONFIG_LWIP 1)
set(CONFIG_LWIP_IPERF 1)
set(CONFIG_LWIP_STRERR 1)
set(CONFIG_LWIP_NETDB 1)
set(CONFIG_LWIP_SOCKET_API 1)

set(CONFIG_ENET_PHY 1)
if (BOARD STREQUAL "hpm6750evk2")
    set(APP_USE_ENET_PORT_COUNT 2)
else()
set(APP_USE_ENET_PORT_COUNT 1)
endif()

#set(APP_USE_ENET_ITF_RGMII 1)
#set(APP_USE_ENET_ITF_RMII 1)
#set(APP_USE_ENET_PHY_DP83867 1)
#set(APP_USE_ENET_PHY_RTL8211 1)
#set(APP_USE_ENET_PHY_DP83848 1)
#set(APP_USE_ENET_PHY_RTL8201 1)

if(NOT DEFINED APP_USE_ENET_PORT_COUNT)
    message(FATAL_ERROR "APP_USE_ENET_PORT_COUNT is undefined!")
endif()


set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_RNDIS 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

if (APP_USE_ENET_PORT_COUNT EQUAL 1)
    sdk_compile_definitions(-DUSE_ENET_PORT_COUNT=1)
elseif(APP_USE_ENET_PORT_COUNT EQUAL 2)
    sdk_compile_definitions(-DUSE_ENET_PORT_COUNT=2)
    sdk_compile_definitions(-DENET_MULTIPLE_PORT=1)
endif()

sdk_compile_definitions(-D__DISABLE_AUTO_NEGO=0)
sdk_compile_definitions(-DLWIP_DHCP=1)
sdk_compile_definitions(-DLWIP_TIMEVAL_PRIVATE=0)
sdk_compile_definitions(-DUSE_LWIPOPTS_APP_H=1)
sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)

sdk_compile_definitions(-DCONFIG_IPERF_TCP_SERVER_PORT=5001)
# sdk_compile_definitions(-DconfigTOTAL_HEAP_SIZE=36864)

# sdk_compile_definitions(-DIPERF_TEST_ENABLED=1)

project(usbnic_eth_multi_net_lwip_example)
sdk_inc(inc)
sdk_inc(rndis_device)
sdk_inc(common/apps/dhcp-server)
sdk_inc(common/apps/dns-server)
sdk_inc(common/apps/ping)

sdk_app_src(rndis_device/cdc_rndis_device.c)
sdk_app_src(rndis_device/cdc_rndis_lwip.c)
sdk_app_src(common/apps/dhcp-server/dhserver.c)
sdk_app_src(common/apps/dns-server/dnserver.c)
sdk_app_src(common/apps/ping/ping.c)

if(APP_USE_ENET_PORT_COUNT EQUAL 1)
    sdk_inc(ports/freertos/single)
    sdk_inc(ports/freertos/single/arch)
    sdk_inc(common/single)
    sdk_app_src(ports/freertos/single/arch/sys_arch.c)
    sdk_app_src(ports/freertos/single/ethernetif.c)
    sdk_app_src(common/single/common.c)
    sdk_app_src(common/single/netconf.c)
elseif(APP_USE_ENET_PORT_COUNT EQUAL 2)
    sdk_inc(ports/freertos/multiple)
    sdk_inc(ports/freertos/multiple/arch)
    sdk_inc(common/multiple)
    sdk_app_src(ports/freertos/multiple/arch/sys_arch.c)
    sdk_app_src(ports/freertos/multiple/ethernetif.c)
    sdk_app_src(common/multiple/common.c)
sdk_app_src(common/multiple/netconf.c)
endif()

sdk_app_src(src/main.c)

generate_ide_projects()
