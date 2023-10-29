# Copyright (c) 2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

string(TOUPPER ${CONFIG_WIZNET} CONFIG_WEZNET_UPPER)
string(TOLOWER ${CONFIG_WIZNET} CONFIG_WEZNET_LOWER)

if(${CONFIG_WEZNET_UPPER} STREQUAL "W5100")
    sdk_compile_definitions(-D_WIZCHIP_=5100)
    sdk_compile_definitions(-D_WIZCHIP_STR_="w5100.h")
elseif(${CONFIG_WEZNET_UPPER} STREQUAL "W5100s")
    sdk_compile_definitions(-D_WIZCHIP_=5105)
    sdk_compile_definitions(-D_WIZCHIP_STR_="w5100S.h")
elseif(${CONFIG_WEZNET_UPPER} STREQUAL "W5200")
    sdk_compile_definitions(-D_WIZCHIP_=5200)
    sdk_compile_definitions(-D_WIZCHIP_STR_="w5200.h")
elseif(${CONFIG_WEZNET_UPPER} STREQUAL "W5300")
    sdk_compile_definitions(-D_WIZCHIP_=5300)
    sdk_compile_definitions(-D_WIZCHIP_STR_="w5300.h")
elseif(${CONFIG_WEZNET_UPPER} STREQUAL "W5500")
    sdk_compile_definitions(-D_WIZCHIP_=5500)
    sdk_compile_definitions(-D_WIZCHIP_STR_="w5500.h")
else()
    message(FATAL_ERROR "${CONFIG_WIZNET} is not supported, only \"W5100\", \"W5100s\", \"W5200\", \"W5300\", \"W5500\"  is supported")
endif()

sdk_inc(.)
sdk_src(socket.c)
sdk_src(wizchip_conf.c)
add_subdirectory(${CONFIG_WEZNET_UPPER})


