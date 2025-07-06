/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "lvgl.h"
#include "lvgl_disp.h"
#include "board.h"
#include "spi_tft_lcd_common.h"
#include "nv3007_display.h"
#include "st7789_display.h"
#include "hpm_clock_drv.h"
#include "te_detection.h"
#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
#include <FreeRTOS.h>
#include <task.h>
#include "queue.h"
#include "semphr.h"
#endif

#define HPM_LVGL_MCHTMR      HPM_MCHTMR
#define HPM_LVGL_MCHTMR_CLK  clock_mchtmr0


#define SPI_TFT_LCD_SPI_BASE  BOARD_APP_SPI_BASE
#define SPI_TFT_LCD_SPI_CLK   (44000000u)

#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
#include "src/core/lv_refr_private.h"
#include "src/display/lv_display_private.h"
#include "hpm_mchtmr_drv.h"
lv_display_t *lcd_disp;

#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
#include "hpm_gptmr_drv.h"
#define APP_BOARD_GPTMR               BOARD_GPTMR
#define APP_BOARD_GPTMR_CH            BOARD_GPTMR_CHANNEL
#define APP_BOARD_GPTMR_IRQ           BOARD_GPTMR_IRQ
#define APP_BOARD_GPTMR_CLOCK         BOARD_GPTMR_CLK_NAME
#define APP_TICK_MS                   (5)
static void timer_config(void);

static volatile bool timer_flag;
lv_disp_drv_t disp_drv;
lv_disp_t *lcd_disp;
lv_disp_draw_buf_t draw_buf;

SDK_DECLARE_EXT_ISR_M(APP_BOARD_GPTMR_IRQ, tick_ms_isr)
void tick_ms_isr(void)
{
    if (gptmr_check_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH))) {
        gptmr_clear_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH));
        lv_tick_inc(APP_TICK_MS);
    }
}

#endif

volatile bool lcd_bus_busy = false;
spi_tft_lcd_context_t spi_tft_ctx;

void spi_txdma_complete_callback(uint32_t channel)
{
    (void)channel;
    lcd_bus_busy = false;
#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
    lv_display_flush_ready(lcd_disp);
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
    lv_disp_flush_ready(&disp_drv);
#endif
    spi_tft_lcd_set_lvgl_test_tx_pin(&spi_tft_ctx, false);
#if ((LVGL_USE_TE_SYNC== 1) && (LVGL_USE_TE_REFR == 0))
    spi_tft_lcd_set_te_interrupt(&spi_tft_ctx, true);
#endif
}


static void lcd_panel_init(void)
{
    memset(&spi_tft_ctx, 0, sizeof(spi_tft_ctx));
    spi_tft_ctx.spi_base =  SPI_TFT_LCD_SPI_BASE;
    spi_tft_ctx.spi_sclk_freq = SPI_TFT_LCD_SPI_CLK;
    spi_tft_ctx.tx_complete = spi_txdma_complete_callback;
    spi_tft_ctx.dc_pin = SPI_TFT_LCD_DC_PIN;
    spi_tft_ctx.rst_pin = SPI_TFT_LCD_RST_PIN;
    spi_tft_ctx.use_bl = false;
    spi_tft_ctx.use_soft_cs = false;
#if defined(SPI_LVGL_TEST_TX_PIN)
    spi_tft_ctx.use_lvgl_test_tx = true;
    spi_tft_ctx.lvgl_test_tx_pin = SPI_LVGL_TEST_TX_PIN;
#endif
#if defined(SPI_LVGL_REFR_TIMER_PIN)
    spi_tft_ctx.use_lvgl_refr_timer = true;
    spi_tft_ctx.lvgl_refr_timer_pin = SPI_LVGL_REFR_TIMER_PIN;
#endif
#if (LVGL_USE_TE_SYNC== 1)
    spi_tft_ctx.use_te = true;
    spi_tft_ctx.te_irq = SPI_TFT_LCD_TE_IRQ;
    spi_tft_ctx.te_pin = SPI_TFT_LCD_TE_PIN;
    spi_tft_ctx.te_priority = 6;
    spi_tft_ctx.te_trigger = gpio_interrupt_trigger_edge_falling;
#else
    spi_tft_ctx.use_te = false;
#endif

#if defined(ENABLE_NV3007_LCD_DRIVER) && (ENABLE_NV3007_LCD_DRIVER == 1)
    nv3007_display_init(&spi_tft_ctx);
#elif defined(ENABLE_ST7789_LCD_DRIVER) && (ENABLE_ST7789_LCD_DRIVER == 1)
    st7789_display_init(&spi_tft_ctx);
#endif
}

#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
ATTR_RAMFUNC uint32_t hpm_lvgl_tick_get_cb(void)
{
    static uint32_t mchtmr_freq_in_khz = 0;

    if (!mchtmr_freq_in_khz) {
        clock_add_to_group(HPM_LVGL_MCHTMR_CLK, 0);
        mchtmr_freq_in_khz = clock_get_frequency(HPM_LVGL_MCHTMR_CLK) / 1000;
    }

    return mchtmr_get_count(HPM_LVGL_MCHTMR) / mchtmr_freq_in_khz;
}

void hpm_lvgl_tick_init(void)
{
    lv_tick_set_cb(hpm_lvgl_tick_get_cb);
}

void hpm_lvgl_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)disp;
#if ((LVGL_USE_TE_SYNC== 1) && (LVGL_USE_TE_REFR == 0))
    while (get_te_detection_status() == false);
    set_te_detection_status(false);
    spi_tft_lcd_set_te_interrupt(&spi_tft_ctx, false);
#endif
    spi_tft_lcd_set_lvgl_test_tx_pin(&spi_tft_ctx, true);
    uint32_t pix_size = lv_color_format_get_size(lv_display_get_color_format(lcd_disp));
    unsigned int size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * pix_size;
    if (spi_tft_ctx.address_set != NULL && spi_tft_ctx.write_ram_nonblocking != NULL) {
        while (lcd_bus_busy);   /* wait until previous transfer is finished */
        spi_tft_ctx.address_set(area->x1, area->y1, area->x2, area->y2);
        spi_tft_ctx.write_ram_nonblocking(16, (uint8_t *)px_map, size);
        lcd_bus_busy = true;
    }
}
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)

static void timer_config(void)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    clock_add_to_group(APP_BOARD_GPTMR_CLOCK, 0);
    gptmr_channel_get_default_config(APP_BOARD_GPTMR, &config);
    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    config.reload = gptmr_freq / 1000 * APP_TICK_MS;
    gptmr_channel_config(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH, &config, false);
    gptmr_start_counter(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);

    gptmr_enable_irq(APP_BOARD_GPTMR, GPTMR_CH_RLD_IRQ_MASK(APP_BOARD_GPTMR_CH));
    intc_m_enable_irq_with_priority(APP_BOARD_GPTMR_IRQ, 1);
}

static void hpm_lvgl_display_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void)disp_drv;
    unsigned int size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * 2;
    if (spi_tft_ctx.address_set != NULL && spi_tft_ctx.write_ram_nonblocking != NULL) {
        while (lcd_bus_busy);   /* wait until previous transfer is finished */
        spi_tft_ctx.address_set(area->x1, area->y1, area->x2, area->y2);
        spi_tft_ctx.write_ram_nonblocking(16, (uint8_t *)color_p, size);
        lcd_bus_busy = true;
    }
}
#endif


void lvgl_disp_init(uint8_t *disp_buf1, uint8_t *disp_buf2, uint32_t size_in_byte)
{
    lcd_panel_init();
#if (LVGL_USE_FULL_BUFFER== 1)
    assert(size_in_byte >= (spi_tft_ctx.width * spi_tft_ctx.height * spi_tft_ctx.pixel_in_byte));
#endif
    lv_init();
#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
    hpm_lvgl_tick_init();
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
    timer_config();
#endif
#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
    lv_display_render_mode_t render_mode = LV_DISPLAY_RENDER_MODE_FULL;
    /* Create the LVGL display object and the LCD display driver */
    lcd_disp = lv_display_create(spi_tft_ctx.width,spi_tft_ctx.height);
    //lv_display_set_rotation(lcd_disp, LV_DISPLAY_ROTATION_270);
#if (LVGL_USE_FULL_BUFFER== 1)
#if (LVGL_USE_DIRECT_MODE== 1)
    render_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
#endif
#else
    render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
#endif
    lv_display_set_buffers(lcd_disp, disp_buf1, disp_buf2, size_in_byte , render_mode);
    lv_display_set_flush_cb(lcd_disp, hpm_lvgl_display_flush_cb);
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
    lv_disp_draw_buf_init(&draw_buf, disp_buf1, disp_buf2, size_in_byte / 2);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = spi_tft_ctx.width;
    disp_drv.ver_res = spi_tft_ctx.height;
#if (LVGL_USE_FULL_BUFFER== 1)
    disp_drv.full_refresh = true;
#if (LVGL_USE_DIRECT_MODE== 1)
    disp_drv.direct_mode = true;
#endif
#else
    disp_drv.full_refresh = false;
    lcd_disp.direct_mode = false;
#endif
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = hpm_lvgl_display_flush_cb;
    lv_disp_t *lcd_disp = lv_disp_drv_register(&disp_drv);
#endif
#if (LVGL_USE_TE_SYNC== 1)
#if (LVGL_USE_TE_REFR == 1)
    lv_timer_del(lcd_disp->refr_timer);
    lcd_disp->refr_timer = NULL;
#endif
#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)

#endif
    spi_tft_lcd_set_te_interrupt(&spi_tft_ctx, true);
    te_detection_init();
#endif
}

void lvgl_disp_te_handle(void)
{
    spi_tft_lcd_set_lvgl_test_refr_timer_pin(&spi_tft_ctx, true);
 #if (LVGL_USE_TE_SYNC== 1)
    spi_tft_lcd_set_te_interrupt(&spi_tft_ctx, false);
#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
    lv_display_refr_timer(NULL);
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
    _lv_disp_refr_timer(NULL);
#endif
    spi_tft_lcd_set_te_interrupt(&spi_tft_ctx, true);
#endif
    spi_tft_lcd_set_lvgl_test_refr_timer_pin(&spi_tft_ctx, false);
}