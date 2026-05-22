/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * WS2812 configuration for hpm5301evklite
 * - SPI1 MOSI drives WS2812 DIN (pad defined via CMake WS2812_DIN)
 * - 16x16 matrix mode (256 LEDs)
 */

#ifndef WS2812_CONF_H
#define WS2812_CONF_H

#define WS2812_USE_SPI          1

/* SPI1 on hpm5301evklite */
#define WS2812_SPI              HPM_SPI1
#define WS2812_SPI_CLCOK        clock_spi1
/* WS2812_DIN is defined via CMakeLists.txt sdk_compile_definitions */

/* 16x16 matrix */
#define WS2812_LED_CONNECT      WS2812_CONNECT_MATRIX
#define WS2812_LED_COL          16
#define WS2812_LED_ROW          16
#define WS2812_LED_NUM          (WS2812_LED_COL * WS2812_LED_ROW)

#endif /* WS2812_CONF_H */
