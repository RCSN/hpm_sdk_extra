/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Ripple effect mode
 * Hand detection triggers 2D ripple animation on LED matrix
 * Ripple expands outward from hand position
 * Multiple ripples can overlap for visual effect
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "WS2812.h"
#include "gesture_engine.h"

#define MAX_RIPPLES         4
#define RIPPLE_MAX_RADIUS   12
#define RIPPLE_SPEED        0.3f    /* Radius growth per frame */
#define RIPPLE_WIDTH        2.0f    /* Ring thickness */
#define RIPPLE_BRIGHTNESS   200

typedef struct {
    float cx, cy;           /* Center position (LED coords) */
    float radius;           /* Current radius */
    bool active;
    uint8_t hue;            /* Color of this ripple */
} ripple_t;

static ripple_t ripples[MAX_RIPPLES];
static int ripple_idx = 0;
static bool was_present = false;
static uint8_t next_hue = 0;
static uint32_t frame_counter = 0;

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

static void spawn_ripple(float cx, float cy)
{
    ripple_t *r = &ripples[ripple_idx];
    r->cx = cx;
    r->cy = cy;
    r->radius = 0;
    r->active = true;
    r->hue = next_hue;
    next_hue += 40; /* Shift hue for each new ripple */
    ripple_idx = (ripple_idx + 1) % MAX_RIPPLES;
}

void app_ripple_init(void)
{
    for (int i = 0; i < MAX_RIPPLES; i++) {
        ripples[i].active = false;
    }
    ripple_idx = 0;
    was_present = false;
    next_hue = 0;
    frame_counter = 0;
}

void app_ripple_process(gesture_engine_t *engine)
{
    const centroid_t *ca = gesture_engine_get_centroid_a(engine);

    frame_counter++;

    /* Trigger new ripple when hand first appears */
    if (ca->present && !was_present) {
        float led_cx = ca->cx * 2.0f;
        float led_cy = ca->cy * 2.0f;
        spawn_ripple(led_cx, led_cy);
    }
    was_present = ca->present;

    /* Clear all LEDs */
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0, 0, 0);
    }

    /* Update and draw ripples */
    for (int ri = 0; ri < MAX_RIPPLES; ri++) {
        ripple_t *rp = &ripples[ri];
        if (!rp->active) continue;

        /* Grow radius */
        rp->radius += RIPPLE_SPEED;

        /* Deactivate if too large */
        if (rp->radius > RIPPLE_MAX_RADIUS) {
            rp->active = false;
            continue;
        }

        /* Fade out as ripple expands */
        float fade = 1.0f - (rp->radius / RIPPLE_MAX_RADIUS);
        uint8_t brightness = (uint8_t)(RIPPLE_BRIGHTNESS * fade);

        /* Draw ripple ring */
        for (int row = 0; row < 16; row++) {
            for (int col = 0; col < 16; col++) {
                float dx = col - rp->cx;
                float dy = row - rp->cy;
                float dist = sqrtf(dx * dx + dy * dy);

                /* Check if this pixel is within the ripple ring */
                float ring_dist = fabsf(dist - rp->radius);
                if (ring_dist < RIPPLE_WIDTH) {
                    /* Intensity falls off from ring center */
                    float intensity = 1.0f - (ring_dist / RIPPLE_WIDTH);
                    uint8_t pixel_brightness = (uint8_t)(brightness * intensity);
                    uint8_t r, g, b;
                    hsv_to_rgb(rp->hue, 255, pixel_brightness, &r, &g, &b);

                    int actual_col = (row % 2 == 0) ? col : (15 - col);
                    int led_idx = row * 16 + actual_col;
                    /* Additive blend with existing pixel */
                    WS2812_MixPixel(led_idx, r, g, b);
                }
            }
        }
    }

    /* If hand is present, show a dim indicator at centroid */
    if (ca->present) {
        float led_cx = ca->cx * 2.0f;
        float led_cy = ca->cy * 2.0f;
        int cx_int = (int)(led_cx + 0.5f);
        int cy_int = (int)(led_cy + 0.5f);
        if (cx_int >= 0 && cx_int < 16 && cy_int >= 0 && cy_int < 16) {
            int actual_col = (cy_int % 2 == 0) ? cx_int : (15 - cx_int);
            int led_idx = cy_int * 16 + actual_col;
            WS2812_SetPixel(led_idx, 80, 80, 80);
        }
    }

    ws2812_mark_dirty();
}
