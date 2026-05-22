/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Gesture light following mode
 * Hand centroid position -> LED cursor on 16x16 matrix
 * Distance -> brightness and color
 * Gesture events change cursor style
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "WS2812.h"
#include "gesture_engine.h"

/* Cursor size and fade parameters */
#define CURSOR_RADIUS       2
#define FADE_RADIUS         5
#define CURSOR_BRIGHTNESS   255
#define TRAIL_LENGTH        5

static float prev_cx = -1, prev_cy = -1;
static uint8_t hue_offset = 0;
static uint32_t frame_counter = 0;

/* HSV to RGB (simplified for embedded) */
static void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region, remainder, p, q, t;
    if (s == 0) { *r = *g = *b = v; return; }
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

void app_gesture_light_init(void)
{
    prev_cx = -1;
    prev_cy = -1;
    hue_offset = 0;
    frame_counter = 0;
}

void app_gesture_light_process(gesture_engine_t *engine)
{
    const centroid_t *ca = gesture_engine_get_centroid_a(engine);
    const gesture_event_t *gesture = gesture_engine_get_last_gesture(engine);
    frame_counter++;

    /* Fade existing pixels (dim by 10% each frame for trail effect) */
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_MixPixel(i, 0, 0, 0); /* Average with black = dim */
    }

    if (!ca->present) {
        ws2812_mark_dirty();
        return;
    }

    /* Map centroid (0-7) to LED position (0-15), scale by 2 */
    float led_cx = ca->cx * 2.0f;
    float led_cy = ca->cy * 2.0f;

    /* Shift hue on gesture */
    if (gesture->type == GESTURE_SWIPE_LEFT) {
        hue_offset -= 30;
    } else if (gesture->type == GESTURE_SWIPE_RIGHT) {
        hue_offset += 30;
    }

    /* Color based on distance: near=warm, far=cool + hue offset */
    uint8_t base_hue = hue_offset;
    if (ca->avg_distance > 0 && ca->avg_distance < 1500) {
        base_hue += (uint8_t)(ca->avg_distance * 150 / 1500);
    }

    /* Draw cursor: bright center with soft glow */
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            float dx = col - led_cx;
            float dy = row - led_cy;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < CURSOR_RADIUS) {
                /* Bright center */
                uint8_t r, g, b;
                hsv_to_rgb(base_hue, 255, CURSOR_BRIGHTNESS, &r, &g, &b);
                int actual_col = (row % 2 == 0) ? col : (15 - col);
                int led_idx = row * 16 + actual_col;
                WS2812_SetPixel(led_idx, r, g, b);
            } else if (dist < FADE_RADIUS) {
                /* Soft glow */
                uint8_t brightness = (uint8_t)(CURSOR_BRIGHTNESS *
                    (1.0f - (dist - CURSOR_RADIUS) / (FADE_RADIUS - CURSOR_RADIUS)));
                uint8_t r, g, b;
                hsv_to_rgb(base_hue, 200, brightness, &r, &g, &b);
                int actual_col = (row % 2 == 0) ? col : (15 - col);
                int led_idx = row * 16 + actual_col;
                WS2812_MixPixel(led_idx, r, g, b);
            }
        }
    }

    prev_cx = led_cx;
    prev_cy = led_cy;

    ws2812_mark_dirty();
}
