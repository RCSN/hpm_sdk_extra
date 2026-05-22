/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Gesture recognition engine for dual VL53L5CX sensors
 * - Centroid tracking + velocity state machine
 * - Supports: left/right/up/down swipe, push/pull, hover, none
 */

#ifndef GESTURE_ENGINE_H
#define GESTURE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "vl53l5cx_api.h"

#define GESTURE_ZONE_ROWS       8
#define GESTURE_ZONE_COLS       8
#define GESTURE_ZONES_PER_SENSOR (GESTURE_ZONE_ROWS * GESTURE_ZONE_COLS)
#define GESTURE_TRAIL_LENGTH    8
#define GESTURE_HISTORY_FRAMES  3

/* Distance threshold for presence detection (mm) */
#define GESTURE_PRESENCE_DIST   1500
/* Min distance to consider valid (mm) */
#define GESTURE_MIN_DIST        30

/* Gesture types */
typedef enum {
    GESTURE_NONE = 0,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN,
    GESTURE_PUSH,          /* Hand moving toward sensor */
    GESTURE_PULL,          /* Hand moving away from sensor */
    GESTURE_HOVER,         /* Hand staying still */
} gesture_type_t;

/* Gesture event with direction and speed */
typedef struct {
    gesture_type_t type;
    float speed;            /* Speed of gesture (pixels/frame) */
    float cx, cy;           /* Centroid position when gesture completed */
    float avg_distance;     /* Average distance of hand (mm) */
} gesture_event_t;

/* Centroid position for one frame */
typedef struct {
    float cx, cy;           /* Centroid position (0.0 - 7.0) */
    float avg_distance;     /* Average distance of hand (mm) */
    bool present;           /* Whether hand is detected */
    float area;             /* Total area (number of zones with hand) */
} centroid_t;

/* Gesture state machine */
typedef enum {
    GESTURE_STATE_IDLE,
    GESTURE_STATE_TRACKING,
    GESTURE_STATE_CONFIRMING,
} gesture_state_t;

/* Per-sensor tracking data */
typedef struct {
    centroid_t trail[GESTURE_TRAIL_LENGTH];
    int trail_idx;
    int trail_count;
    centroid_t prev_centroid;
    gesture_state_t state;
    int confirm_count;
    float velocity_x, velocity_y;
} sensor_tracker_t;

/* Gesture engine context */
typedef struct {
    VL53L5CX_Configuration *sensor_a;
    VL53L5CX_Configuration *sensor_b;
    VL53L5CX_ResultsData results_a;
    VL53L5CX_ResultsData results_b;
    sensor_tracker_t tracker_a;
    sensor_tracker_t tracker_b;
    gesture_event_t last_gesture;
    uint32_t frame_count;
    /* Background reference (for subtraction) */
    int16_t background_a[GESTURE_ZONES_PER_SENSOR];
    int16_t background_b[GESTURE_ZONES_PER_SENSOR];
    bool background_valid;
} gesture_engine_t;

/**
 * @brief Initialize gesture engine with two VL53L5CX sensor configs
 */
void gesture_engine_init(gesture_engine_t *engine,
                         VL53L5CX_Configuration *sensor_a,
                         VL53L5CX_Configuration *sensor_b);

/**
 * @brief Initialize both VL53L5CX sensors (8x8 @ 15Hz)
 * @return 0 on success, -1 on failure
 */
int gesture_engine_sensors_init(gesture_engine_t *engine);

/**
 * @brief Capture background reference (call when no hand present)
 */
void gesture_engine_capture_background(gesture_engine_t *engine);

/**
 * @brief Process one frame from both sensors and detect gestures
 * @return true if a new gesture event was detected
 */
bool gesture_engine_process(gesture_engine_t *engine);

/**
 * @brief Get the last detected gesture event
 */
const gesture_event_t *gesture_engine_get_last_gesture(const gesture_engine_t *engine);

/**
 * @brief Get current centroid data for sensor A
 */
const centroid_t *gesture_engine_get_centroid_a(const gesture_engine_t *engine);

/**
 * @brief Get current centroid data for sensor B
 */
const centroid_t *gesture_engine_get_centroid_b(const gesture_engine_t *engine);

/**
 * @brief Get raw distance data for sensor A (64 zones)
 */
const int16_t *gesture_engine_get_distance_a(const gesture_engine_t *engine);

/**
 * @brief Get raw distance data for sensor B (64 zones)
 */
const int16_t *gesture_engine_get_distance_b(const gesture_engine_t *engine);

/**
 * @brief Convert gesture type to string
 */
const char *gesture_type_str(gesture_type_t type);

/**
 * @brief Mark WS2812 buffer as dirty for timer-driven non-blocking refresh.
 * App modes should call this instead of WS2812_Update(true).
 * The GPTMR refresh timer in main.c handles the actual SPI DMA transfer.
 */
void ws2812_mark_dirty(void);

#endif /* GESTURE_ENGINE_H */
