/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Depth heatmap mode
 * Maps VL53L5CX 8x8 distance data to 16x16 LED matrix
 * Each ToF zone -> 2x2 LED pixels
 * Distance -> HSV color band (near=warm, far=cool)
 */

#include <stdio.h>
#include <stdint.h>
#include "WS2812.h"
#include "gesture_engine.h"

/* Max display distance (mm) */
#define HEATMAP_MAX_DIST    1500
/* Min display distance (mm) */
#define HEATMAP_MIN_DIST    30

/* HSV to RGB conversion for heatmap */
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        *r = *g = *b = v;
        return;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
    case 0: *r = v;  *g = t;  *b = p;  break;
    case 1: *r = q;  *g = v;  *b = p;  break;
    case 2: *r = p;  *g = v;  *b = t;  break;
    case 3: *r = p;  *g = q;  *b = v;  break;
    case 4: *r = t;  *g = p;  *b = v;  break;
    default: *r = v; *g = p;  *b = q;  break;
    }
}

/* Distance to color: near=red(0), far=blue(200) */
static void distance_to_color(int16_t dist, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (dist < HEATMAP_MIN_DIST || dist > HEATMAP_MAX_DIST) {
        *r = *g = *b = 0; /* Out of range = black */
        return;
    }

    /* Map distance to hue: near=0(red), far=200(blue-violet) */
    uint8_t hue = (uint8_t)((dist - HEATMAP_MIN_DIST) * 200 / (HEATMAP_MAX_DIST - HEATMAP_MIN_DIST));
    /* Invert: near=hue 200 (warm), far=hue 0 (cool) for more intuitive feel */
    hue = 200 - hue;

    uint8_t brightness = 200;
    /* Dim very distant objects */
    if (dist > HEATMAP_MAX_DIST * 8 / 10) {
        brightness = 200 * (HEATMAP_MAX_DIST - dist) / (HEATMAP_MAX_DIST / 5);
    }

    hsv_to_rgb(hue, 255, brightness, r, g, b);
}

void app_depth_heatmap_init(void)
{
    /* Nothing special to init */
}

void app_depth_heatmap_process(gesture_engine_t *engine)
{
    const int16_t *dist_a = gesture_engine_get_distance_a(engine);
    const int16_t *dist_b = gesture_engine_get_distance_b(engine);

    /*
     * Map 2 sensors x 8x8 zones to 16x16 LED matrix:
     * Sensor A -> left half (columns 0-7)
     * Sensor B -> right half (columns 8-15)
     * Each zone maps to 2x2 LED pixels
     */
    for (int zr = 0; zr < 8; zr++) {
        for (int zc = 0; zc < 8; zc++) {
            int zone_idx = zr * 8 + zc;

            /* Sensor A: left half */
            uint8_t r, g, b;
            distance_to_color(dist_a[zone_idx], &r, &g, &b);

            /* Map to 2x2 pixels in LED matrix */
            int led_col = zc * 2;
            int led_row = zr * 2;

            /* WS2812 matrix uses snake pattern: odd rows reversed */
            for (int dr = 0; dr < 2; dr++) {
                for (int dc = 0; dc < 2; dc++) {
                    int col = led_col + dc;
                    int row = led_row + dr;
                    /* Snake pattern: even rows L->R, odd rows R->L */
                    int actual_col = (row % 2 == 0) ? col : (15 - col);
                    int led_idx = row * 16 + actual_col;
                    WS2812_SetPixel(led_idx, r, g, b);
                }
            }

            /* Sensor B: right half */
            distance_to_color(dist_b[zone_idx], &r, &g, &b);

            led_col = zc * 2 + 8; /* Offset to right half */

            for (int dr = 0; dr < 2; dr++) {
                for (int dc = 0; dc < 2; dc++) {
                    int col = led_col + dc;
                    int row = led_row + dr;
                    int actual_col = (row % 2 == 0) ? col : (15 - col);
                    int led_idx = row * 16 + actual_col;
                    WS2812_SetPixel(led_idx, r, g, b);
                }
            }
        }
    }

    ws2812_mark_dirty();
}
