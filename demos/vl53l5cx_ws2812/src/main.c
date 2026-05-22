/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * VL53L5CX + WS2812 gesture interactive demo
 * Board: hpm5301evklite
 * - Two VL53L5CX ToF sensors on I2C2/I2C3
 * - 16x16 WS2812 LED matrix on SPI1 MOSI
 *
 * Menu modes:
 *   1 - Depth heatmap (distance -> color)
 *   2 - Gesture light following (centroid -> LED cursor)
 *   3 - Ripple effect (hand triggers ripple animation)
 *   4 - Dual ToF ranging (bar chart + digit readout)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_uart_drv.h"
#include "hpm_dma_mgr.h"
#include "hpm_gptmr_drv.h"
#include "hpm_interrupt.h"
#include "WS2812.h"
#include "vl53l5cx_api.h"
#include "gesture_engine.h"

/* Forward declarations for app modes */
extern void app_depth_heatmap_init(void);
extern void app_depth_heatmap_process(gesture_engine_t *engine);

extern void app_gesture_light_init(void);
extern void app_gesture_light_process(gesture_engine_t *engine);

extern void app_ripple_init(void);
extern void app_ripple_process(gesture_engine_t *engine);

extern void app_dual_tof_init(void);
extern void app_dual_tof_process(gesture_engine_t *engine);

/* VL53L5CX configuration structs (large, placed in non-cacheable) */
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8)
static VL53L5CX_Configuration vl53l5cx_dev_a;

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8)
static VL53L5CX_Configuration vl53l5cx_dev_b;

static gesture_engine_t g_engine;

/* Platform functions defined in vl53l5cx_platform_hpm.c */
extern void vl53l5cx_hpm_gpio_init(void);
extern void vl53l5cx_hpm_sensor_enable(uint8_t sensor_id);
extern void vl53l5cx_hpm_sensor_disable(uint8_t sensor_id);
extern void vl53l5cx_hpm_sensor_reset(uint8_t sensor_id);
extern void vl53l5cx_hpm_i2c_init(void);
extern VL53L5CX_Platform *vl53l5cx_hpm_get_sensor_a_platform(void);
extern VL53L5CX_Platform *vl53l5cx_hpm_get_sensor_b_platform(void);

typedef void (*app_init_func)(void);
typedef void (*app_process_func)(gesture_engine_t *engine);

typedef struct {
    const char *name;
    app_init_func init;
    app_process_func process;
} app_mode_t;

static const app_mode_t app_modes[] = {
    { "Depth Heatmap",    app_depth_heatmap_init,    app_depth_heatmap_process },
    { "Gesture Light",    app_gesture_light_init,    app_gesture_light_process },
    { "Ripple Effect",    app_ripple_init,           app_ripple_process },
    { "Dual ToF Ranging",  app_dual_tof_init,         app_dual_tof_process },
};

#define NUM_MODES (sizeof(app_modes) / sizeof(app_modes[0]))

static int current_mode = 0;

/* WS2812 non-blocking refresh timer (GPTMR0 CH0, ~30fps) */
#define WS2812_REFRESH_GPTMR        HPM_GPTMR0
#define WS2812_REFRESH_GPTMR_CH     0
#define WS2812_REFRESH_GPTMR_CLK    clock_gptmr0
#define WS2812_REFRESH_GPTMR_IRQ    IRQn_GPTMR0
#define WS2812_REFRESH_FPS          30

static volatile bool ws2812_dirty = false;
static volatile bool ws2812_refresh_flag = false;

SDK_DECLARE_EXT_ISR_M(WS2812_REFRESH_GPTMR_IRQ, isr_ws2812_refresh)
void isr_ws2812_refresh(void)
{
    if (gptmr_check_status(WS2812_REFRESH_GPTMR, GPTMR_CH_RLD_STAT_MASK(WS2812_REFRESH_GPTMR_CH))) {
        gptmr_clear_status(WS2812_REFRESH_GPTMR, GPTMR_CH_RLD_STAT_MASK(WS2812_REFRESH_GPTMR_CH));
        if (ws2812_dirty) {
            ws2812_refresh_flag = true;
        }
    }
}

static void ws2812_refresh_timer_init(void)
{
    clock_add_to_group(WS2812_REFRESH_GPTMR_CLK, 0);
    uint32_t gptmr_freq = clock_get_frequency(WS2812_REFRESH_GPTMR_CLK);
    gptmr_channel_config_t config;
    gptmr_channel_get_default_config(WS2812_REFRESH_GPTMR, &config);
    config.reload = gptmr_freq / WS2812_REFRESH_FPS;
    gptmr_channel_config(WS2812_REFRESH_GPTMR, WS2812_REFRESH_GPTMR_CH, &config, false);
    gptmr_start_counter(WS2812_REFRESH_GPTMR, WS2812_REFRESH_GPTMR_CH);
    gptmr_enable_irq(WS2812_REFRESH_GPTMR, GPTMR_CH_RLD_IRQ_MASK(WS2812_REFRESH_GPTMR_CH));
    intc_m_enable_irq_with_priority(WS2812_REFRESH_GPTMR_IRQ, 2);
}

/* Mark WS2812 buffer dirty for next timer-driven refresh (called by app modes too) */
void ws2812_mark_dirty(void)
{
    ws2812_dirty = true;
}

/* Try non-blocking refresh in main loop (triggered by timer ISR) */
static void ws2812_try_refresh(void)
{
    if (ws2812_refresh_flag && !WS2812_IsBusy()) {
        ws2812_refresh_flag = false;
        ws2812_dirty = false;
        WS2812_Update(false);
    }
}

static void print_menu(void)
{
    printf("\r\n");
    printf("=== VL53L5CX + WS2812 Demo ===\r\n");
    printf("1 - Depth Heatmap\r\n");
    printf("2 - Gesture Light Following\r\n");
    printf("3 - Ripple Effect\r\n");
    printf("4 - Dual ToF Ranging\r\n");
    printf("b - Capture background\r\n");
    printf("r - Reset sensors\r\n");
    printf("Select mode: \r\n");
}

static void ws2812_clear_all(void)
{
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0, 0, 0);
    }
    ws2812_mark_dirty();
}

static void ws2812_fill_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, r, g, b);
    }
    ws2812_mark_dirty();
}

/* WS2812 power-on self test: Red -> Green -> Blue -> White -> Off (blocking) */
static void ws2812_self_test(void)
{
    printf("WS2812 self-test: Red...\r\n");
    ws2812_fill_all(32, 0, 0);
    WS2812_Update(true);
    board_delay_ms(500);

    printf("WS2812 self-test: Green...\r\n");
    ws2812_fill_all(0, 32, 0);
    WS2812_Update(true);
    board_delay_ms(500);

    printf("WS2812 self-test: Blue...\r\n");
    ws2812_fill_all(0, 0, 32);
    WS2812_Update(true);
    board_delay_ms(500);

    printf("WS2812 self-test: White...\r\n");
    ws2812_fill_all(32, 32, 32);
    WS2812_Update(true);
    board_delay_ms(500);

    ws2812_clear_all();
    WS2812_Update(true);
    printf("WS2812 self-test: PASS\r\n");
}

int main(void)
{
    board_init();
    printf("\r\nVL53L5CX + WS2812 Gesture Demo\r\n");

    /* Init DMA manager (required by SPI DMA for WS2812) */
    dma_mgr_init();

    /* Init WS2812 */
    printf("Initializing WS2812 (16x16 matrix)...\r\n");
    WS2812_Init();
    ws2812_clear_all();

    /* Init WS2812 self-test timer-driven refresh (30fps, non-blocking) */
    ws2812_refresh_timer_init();

    /* WS2812 self-test: verify LED matrix works */
    ws2812_self_test();

    /* Init VL53L5CX-SATEL control pins (PWR_EN, LPn, I2C_RST) and power-up sensors */
    printf("Powering up VL53L5CX-SATEL breakout boards...\r\n");
    vl53l5cx_hpm_gpio_init();
    vl53l5cx_hpm_sensor_enable(0);  /* Sensor A: PWR_EN -> I2C_RST pulse -> LPn */
    vl53l5cx_hpm_sensor_enable(1);  /* Sensor B: PWR_EN -> I2C_RST pulse -> LPn */

    /* Init I2C for VL53L5CX */
    printf("Initializing I2C for VL53L5CX...\r\n");
    vl53l5cx_hpm_i2c_init();

    /* Setup VL53L5CX device configs */
    vl53l5cx_dev_a.platform = *vl53l5cx_hpm_get_sensor_a_platform();
    vl53l5cx_dev_b.platform = *vl53l5cx_hpm_get_sensor_b_platform();

    /* Init gesture engine */
    gesture_engine_init(&g_engine, &vl53l5cx_dev_a, &vl53l5cx_dev_b);

    /* Init VL53L5CX sensors */
    bool sensors_ok = (gesture_engine_sensors_init(&g_engine) == 0);
    if (!sensors_ok) {
        printf("Sensor init failed! Check connections.\r\n");
        printf("  Sensor A: I2C2 (PB08=SCL, PB09=SDA)\r\n");
        printf("  Sensor B: I2C3 (PB13=SCL, PB12=SDA)\r\n");
        printf("Continuing with WS2812 demo (press 'r' to retry sensors)\r\n");
    }

    /* Capture initial background */
    board_delay_ms(500);
    printf("Capturing background (no hand please)...\r\n");
    gesture_engine_capture_background(&g_engine);

    /* Start default mode */
    print_menu();
    app_modes[current_mode].init();
    printf("Mode: %s\r\n", app_modes[current_mode].name);

    while (1) {
        /* Check for user input */
        uint8_t ch = 0;
        if (uart_get_data_count_in_rx_fifo(BOARD_CONSOLE_UART_BASE) > 0) {
            ch = uart_read_byte(BOARD_CONSOLE_UART_BASE);
        }
        if (ch != 0) {
            switch (ch) {
            case '1': case '2': case '3': case '4':
                current_mode = ch - '1';
                ws2812_clear_all();
                app_modes[current_mode].init();
                printf("Mode: %s\r\n", app_modes[current_mode].name);
                break;
            case 'b':
                printf("Capturing background...\r\n");
                gesture_engine_capture_background(&g_engine);
                break;
            case 'r':
                printf("Resetting sensors...\r\n");
                vl53l5cx_stop_ranging(g_engine.sensor_a);
                vl53l5cx_stop_ranging(g_engine.sensor_b);
                gesture_engine_sensors_init(&g_engine);
                break;
            default:
                print_menu();
                break;
            }
        }

        /* Non-blocking WS2812 refresh driven by GPTMR */
        ws2812_try_refresh();

        /* Process ToF data and update LEDs */
        gesture_engine_process(&g_engine);
        app_modes[current_mode].process(&g_engine);
    }

    return 0;
}
