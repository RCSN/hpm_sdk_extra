/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef ST7789_DISPLAY_H
#define ST7789_DISPLAY_H

#if defined(ENABLE_ST7789_LCD_DRIVER) && (ENABLE_ST7789_LCD_DRIVER == 1)


#include "lvgl.h"
#include "lv_app_conf.h"
#include "spi_tft_lcd_common.h"

#define ADDR_OFFSET    (0)
#if (USE_HORIZONTIAL == 0) || (USE_HORIZONTIAL == 1)
#define ST7789_LCD_V_RES 320
#define ST7789_LCD_H_RES 240
#else
#define ST7789_LCD_V_RES 240
#define ST7789_LCD_H_RES 320
#endif

void st7789_display_init(spi_tft_lcd_context_t *ctx);

#endif

#endif
