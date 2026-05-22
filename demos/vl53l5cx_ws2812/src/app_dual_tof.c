/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Dual ToF ranging mode
 * Reads average distance from two VL53L5CX sensors,
 * prints to console, and displays numbers on 16x16 WS2812 matrix.
 *
 * Layout (top-bottom split):
 *   Rows 0-7:   Sensor A distance (cm), full 16 cols
 *   Rows 8-15:  Sensor B distance (cm), full 16 cols
 *   Each half = 8 rows x 16 cols, 4x6 font, max 3 digits (cm, integer)
 */

#include <stdio.h>
#include <stdint.h>
#include "WS2812.h"
#include "gesture_engine.h"

#define LED_ROWS  16
#define LED_COLS  16

/* Direct mapping: all rows L->R (no snake reversal for number display) */
static int led_idx(int row, int col)
{
    return row * 16 + col;
}

/*
 * 4x6 pixel font for digits 0-9.
 * Each entry is 6 rows of 4 bits (MSB = leftmost pixel).
 */
static const uint8_t font_4x6[10][6] = {
    {0xE, 0x9, 0x9, 0x9, 0x9, 0xE},  /* 0 */
    {0x2, 0x6, 0x2, 0x2, 0x2, 0x7},  /* 1 */
    {0xE, 0x1, 0xE, 0x8, 0x8, 0xF},  /* 2 */
    {0xE, 0x1, 0x2, 0x1, 0x1, 0xE},  /* 3 */
    {0x9, 0x9, 0xF, 0x1, 0x1, 0x1},  /* 4 */
    {0xF, 0x8, 0xE, 0x1, 0x1, 0xE},  /* 5 */
    {0xE, 0x8, 0xE, 0x9, 0x9, 0xE},  /* 6 */
    {0xE, 0x1, 0x2, 0x2, 0x2, 0x2},  /* 7 */
    {0xE, 0x9, 0xE, 0x9, 0x9, 0xE},  /* 8 */
    {0xE, 0x9, 0xF, 0x1, 0x1, 0xE},  /* 9 */
};

static void draw_digit(int d, int row0, int col0, uint8_t r, uint8_t g, uint8_t b)
{
    if (d < 0 || d > 9) return;
    for (int rr = 0; rr < 6; rr++) {
        uint8_t bits = font_4x6[d][rr];
        for (int cc = 0; cc < 4; cc++) {
            if (bits & (1 << cc)) {
                int row = row0 + rr;
                int col = col0 + cc;
                if (row < LED_ROWS && col < LED_COLS) {
                    WS2812_SetPixel(led_idx(row, col), r, g, b);
                }
            }
        }
    }
}

/*
 * Display distance value (mm) across full 16 cols.
 * row0 = starting row (0 for A, 8 for B).
 * Each digit = 4px wide + 1px gap. 4 digits = 4*5-1 = 19px.
 * Center in 16 cols: start at (16 - width)/2.
 * For 4 digits: width=19, start=-1 → left-align at col 0.
 * Use 1px gap: 4+1+4+1+4+1+4 = 19. Shift left to col 0, last digit ends at col 18.
 * So for 4-digit values, just start at col 0, 1px gap, fits.
 */
static void display_distance_cm(int16_t val_mm, int row0, uint8_t r, uint8_t g, uint8_t b)
{
    /* Convert mm to cm (integer, no decimal) */
    int val_cm = val_mm / 10;
    if (val_cm < 0) val_cm = 0;
    if (val_cm > 999) val_cm = 999;

    int digits[3];
    int n = 0;

    if (val_cm == 0) {
        digits[0] = 0; n = 1;
    } else {
        int tmp = val_cm;
        while (tmp > 0 && n < 3) {
            digits[n] = tmp % 10;
            tmp /= 10;
            n++;
        }
    }

    /* Each digit = 4px wide, 1px gap. Total width = n*5 - 1 */
    int total_w = n * 5 - 1;
    int start_col = (16 - total_w) / 2;  /* Center */

    /* Draw digits, row0+1 to leave top row for label */
#if 0
    int draw_row = row0 + 1;
    for (int i = 0; i < n; i++) {
        draw_digit(digits[n - 1 - i], draw_row, start_col + i * 5, r, g, b);
    }
#endif
}

/* Compute average distance from valid zones */
static int16_t avg_distance(const int16_t *zones, int count)
{
    int32_t sum = 0;
    int valid = 0;
    for (int i = 0; i < count; i++) {
        if (zones[i] > 0 && zones[i] < 4000) {
            sum += zones[i];
            valid++;
        }
    }
    return (valid > 0) ? (int16_t)(sum / valid) : 0;
}

void app_dual_tof_init(void)
{
    printf("Dual ToF Ranging: press 4, distance in cm on LED + printed\r\n");
}

static uint32_t print_counter = 0;

void app_dual_tof_process(gesture_engine_t *engine)
{
    const int16_t *dist_a = gesture_engine_get_distance_a(engine);
    const int16_t *dist_b = gesture_engine_get_distance_b(engine);

    int16_t avg_a = avg_distance(dist_a, 64);
    int16_t avg_b = avg_distance(dist_b, 64);

    /* Print every ~1 second (at 15fps, every 15 frames) */
    print_counter++;
    if (print_counter % 15 == 0) {
        printf("ToF A: %d mm (%d cm)  |  ToF B: %d mm (%d cm)\r\n",
               avg_a, avg_a / 10, avg_b, avg_b / 10);
    }
#if 0
    /* Clear matrix */
    for (int i = 0; i < LED_ROWS * LED_COLS; i++) {
        WS2812_SetPixel(i, 0, 0, 0);
    }

    /* Top half (rows 0-7): Sensor A, green */
    WS2812_SetPixel(led_idx(0, 0), 0, 18, 0);  /* A label */
    display_distance_cm(avg_a, 0, 0, 20, 0);       /* Green */

    /* Bottom half (rows 8-15): Sensor B, cyan */
    WS2812_SetPixel(led_idx(8, 0), 0, 18, 18); /* B label */
    display_distance_cm(avg_b, 8, 0, 20, 20);      /* Cyan */

    /* Separator line between top/bottom halves */
    for (int col = 0; col < LED_COLS; col++) {
        WS2812_SetPixel(led_idx(7, col), 25, 25, 25);
    }

    ws2812_mark_dirty();
#endif
}