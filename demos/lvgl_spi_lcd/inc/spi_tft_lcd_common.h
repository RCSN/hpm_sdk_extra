/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef SPI_TFT_LCD_COMMON_H
#define SPI_TFT_LCD_COMMON_H

#include "hpm_gpio_drv.h"
#include "hpm_spi.h"

typedef void (*tft_spi_complete_cb)(uint32_t channel);

#ifndef SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT
#define SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT (0x10000U)
#endif

#ifndef USE_HORIZONTIAL
#define USE_HORIZONTIAL           0
#endif

typedef struct
{
    SPI_Type *spi_base;
    uint32_t spi_sclk_freq;
    tft_spi_complete_cb tx_complete;
    uint32_t dc_pin;
    uint32_t rst_pin;
    bool use_bl;
    uint32_t bl_pin;
    uint32_t bl_level;
    bool use_soft_cs;
    uint32_t cs_pin;
    uint32_t cs_level;
    bool use_te;
    uint32_t te_pin;
    uint32_t te_irq;
    uint32_t te_priority;
    bool use_lvgl_test_tx;
    uint32_t lvgl_test_tx_pin;
    bool use_lvgl_refr_timer;
    uint32_t lvgl_refr_timer_pin;
    gpio_interrupt_trigger_t te_trigger;
    uint32_t width;
    uint32_t height;
    uint8_t pixel_in_byte;
    void (*fill_color)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
    void (*draw_point)(uint16_t x, uint16_t y, uint16_t color);
    void (*address_set)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void (*write_ram_blocking)(uint8_t bit_width, uint8_t *data, uint32_t size, uint32_t timeout);
    void (*write_ram_nonblocking)(uint8_t bit_width, uint8_t *data, uint32_t size);
} spi_tft_lcd_context_t;

hpm_stat_t spi_tft_lcd_hardware_init(spi_tft_lcd_context_t *ctx);
void spi_tft_lcd_set_dc(spi_tft_lcd_context_t *ctx, bool state);
void spi_tft_lcd_set_rst(spi_tft_lcd_context_t *ctx, bool state);
void spi_tft_lcd_set_bl(spi_tft_lcd_context_t *ctx, bool state);
void spi_tft_lcd_set_cs(spi_tft_lcd_context_t *ctx, bool state);
void spi_tft_lcd_set_lvgl_test_tx_pin_toggle(spi_tft_lcd_context_t *ctx);
void spi_tft_lcd_set_lvgl_test_tx_pin(spi_tft_lcd_context_t *ctx, bool state);
void spi_tft_lcd_set_lvgl_test_refr_timer_pin(spi_tft_lcd_context_t *ctx, bool state);
bool spi_tft_lcd_te_get_state(spi_tft_lcd_context_t *ctx);
void spi_tft_lcd_set_te_interrupt(spi_tft_lcd_context_t *ctx, bool enable);
void spi_tft_lcd_clear_te_interrupt_flag(spi_tft_lcd_context_t *ctx);
hpm_stat_t spi_tft_lcd_transfer_data_blocking(spi_tft_lcd_context_t *ctx, uint8_t bit_width, uint8_t *data, uint32_t size, uint32_t timeout);
hpm_stat_t spi_tft_lcd_transfer_data_nonblocking(spi_tft_lcd_context_t *ctx, uint8_t bit_width, uint8_t *data, uint32_t size);

#endif
