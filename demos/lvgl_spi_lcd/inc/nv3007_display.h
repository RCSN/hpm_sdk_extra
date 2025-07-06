/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NV3007_DISPLAY_H
#define NV3007_DISPLAY_H

#if defined(ENABLE_NV3007_LCD_DRIVER) && (ENABLE_NV3007_LCD_DRIVER == 1)

#include "lvgl.h"
#include "lv_app_conf.h"
#include "spi_tft_lcd_common.h"

#define ADDR_OFFSET    (34)
#if (USE_HORIZONTIAL == 0) || (USE_HORIZONTIAL == 1)
#define NV3007_LCD_V_RES 428
#define NV3007_LCD_H_RES 142
#else
#define NV3007_LCD_V_RES 142
#define NV3007_LCD_H_RES 428
#endif

void nv3007_display_init(spi_tft_lcd_context_t *ctx);

#endif

#endif
