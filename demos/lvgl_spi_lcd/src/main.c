/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "lvgl.h"
#include "hpm_dma_mgr.h"
#include "hpm_spi.h"
#include "lvgl_disp.h"
#include <demos/lv_demos.h>
#include "spi_tft_lcd_common.h"
#include "te_detection.h"
#include "usbh_core.h"
#include "usbh_hid_lvgl.h"

#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
#include <FreeRTOS.h>
#include <task.h>
#endif
#if (LVGL_USE_FULL_BUFFER== 1)
// #define DISP_BUFFER_SIZE     (142 * 428 * 2)
#define DISP_BUFFER_SIZE     (240 * 320 * 2)
#else
#define DISP_BUFFER_SIZE     (240 * 10 * 2)
#endif

#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
#include "src/core/lv_refr_private.h"
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(LV_DRAW_BUF_ALIGN)  uint8_t disp_buf1[DISP_BUFFER_SIZE];
// ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(LV_DRAW_BUF_ALIGN)  uint8_t disp_buf2[DISP_BUFFER_SIZE];
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
ATTR_PLACE_AT_NONCACHEABLE  uint8_t disp_buf1[DISP_BUFFER_SIZE];
// ATTR_PLACE_AT_NONCACHEABLE  uint8_t disp_buf2[DISP_BUFFER_SIZE];

#endif
extern spi_tft_lcd_context_t spi_tft_ctx;
#if (LVGL_USE_TE_SYNC== 1)
SDK_DECLARE_EXT_ISR_M(SPI_TFT_LCD_TE_IRQ, isr_gpio)
void isr_gpio(void)
{
    if (spi_tft_lcd_te_get_state(&spi_tft_ctx) == false) {
        spi_tft_lcd_clear_te_interrupt_flag(&spi_tft_ctx);
        set_te_detection_status(true);
    }
}
#endif

#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
static void lvgl_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t delay;
    while (1) {
        delay = lv_timer_handler();
#if ((LVGL_USE_TE_SYNC== 1) && (LVGL_USE_TE_REFR == 1))
        if (te_come_flag) {
            lvgl_disp_te_handle();
            te_come_flag = false;
        }
#endif
        vTaskDelay(delay);
    }
}
#endif

int main(void)
{
    board_init();
    dma_mgr_init();
    lvgl_disp_init(disp_buf1, NULL, DISP_BUFFER_SIZE);

    board_init_usb((USB_Type *)CONFIG_HPM_USBH_BASE);
    /* set irq priority */
    intc_set_irq_priority(CONFIG_HPM_USBH_IRQn, 1);
    usbh_initialize(0, CONFIG_HPM_USBH_BASE);

    usbh_hid_lvgl_add_mouse(0);
    usbh_hid_lvgl_add_keyboard();

#if LV_USE_DEMO_STRESS
    lv_demo_stress();
#endif
#if LV_USE_DEMO_BENCHMARK
    lv_demo_benchmark();
#endif
#if LV_USE_DEMO_MUSIC
    lv_demo_music();
#endif

#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
#if LV_USE_DEMO_WIDGETS && !LV_USE_DEMO_BENCHMARK
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();
#endif
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
#if LV_USE_DEMO_WIDGETS
    lv_demo_widgets();
#endif
#endif

#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
    if (xTaskCreate(lvgl_task, "lvgl", 2048, NULL, 5, NULL) != pdPASS) {
        printf("Task creation failed!.\n");
        for (;;) {
        }
    }
    vTaskStartScheduler();
#endif

    while (1) {
#if defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 9)
        lv_timer_periodic_handler();
#elif defined(LVGL_MAJOR_VERSION) && (LVGL_MAJOR_VERSION == 8)
        lv_task_handler();
#endif
#if ((LVGL_USE_TE_SYNC== 1) && (LVGL_USE_TE_REFR == 1))
        if (te_come_flag) {
            lvgl_disp_te_handle();
            te_come_flag = false;
        }
#endif
    }

}