/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Gesture recognition engine implementation
 * Centroid tracking + velocity state machine
 */

#include "gesture_engine.h"
#include "board.h"
#include <math.h>
#include <string.h>

/* Velocity threshold to start tracking (zone units per frame) */
#define VELOCITY_THRESHOLD      0.8f
/* Number of consistent frames to confirm gesture */
#define CONFIRM_FRAMES          3
/* Cooldown frames after gesture detected */
#define GESTURE_COOLDOWN        15
/* Hover detection: velocity below this for this many frames */
#define HOVER_VELOCITY          0.3f
#define HOVER_FRAMES            10

static void compute_centroid(const int16_t *distance, const int16_t *background,
                             bool bg_valid, centroid_t *out)
{
    float sum_x = 0, sum_y = 0, sum_d = 0;
    float total_weight = 0;
    int count = 0;

    for (int r = 0; r < GESTURE_ZONE_ROWS; r++) {
        for (int c = 0; c < GESTURE_ZONE_COLS; c++) {
            int idx = r * GESTURE_ZONE_COLS + c;
            int16_t d = distance[idx];

            /* Skip invalid readings */
            if (d <= GESTURE_MIN_DIST || d >= GESTURE_PRESENCE_DIST) {
                continue;
            }

            /* Background subtraction: skip zones that match background */
            if (bg_valid && background != NULL) {
                if (abs(d - background[idx]) < 100) { /* 100mm threshold */
                    continue;
                }
            }

            /* Weight by inverse distance (closer = higher weight) */
            float weight = (float)(GESTURE_PRESENCE_DIST - d) / GESTURE_PRESENCE_DIST;
            sum_x += c * weight;
            sum_y += r * weight;
            sum_d += d;
            total_weight += weight;
            count++;
        }
    }

    if (total_weight > 0.01f && count >= 2) {
        out->cx = sum_x / total_weight;
        out->cy = sum_y / total_weight;
        out->avg_distance = sum_d / count;
        out->present = true;
        out->area = (float)count;
    } else {
        out->cx = -1;
        out->cy = -1;
        out->avg_distance = 0;
        out->present = false;
        out->area = 0;
    }
}

static void tracker_update(sensor_tracker_t *t, const centroid_t *c)
{
    /* Add to trail */
    t->trail[t->trail_idx] = *c;
    t->trail_idx = (t->trail_idx + 1) % GESTURE_TRAIL_LENGTH;
    if (t->trail_count < GESTURE_TRAIL_LENGTH) {
        t->trail_count++;
    }
}

static void compute_velocity(sensor_tracker_t *t, const centroid_t *c)
{
    if (c->present && t->prev_centroid.present) {
        t->velocity_x = c->cx - t->prev_centroid.cx;
        t->velocity_y = c->cy - t->prev_centroid.cy;
    } else {
        t->velocity_x = 0;
        t->velocity_y = 0;
    }
}

static gesture_type_t classify_gesture(float vx, float vy,
                                        const centroid_t *ca, const centroid_t *cb)
{
    float speed = sqrtf(vx * vx + vy * vy);
    (void)cb; /* Use for cross-validation in future */

    if (speed < VELOCITY_THRESHOLD) {
        return GESTURE_NONE;
    }

    /* Primary direction from horizontal sensor (sensor A = left/right) */
    float abs_vx = fabsf(vx);
    float abs_vy = fabsf(vy);

    if (abs_vx > abs_vy * 1.5f) {
        /* Horizontal dominant */
        return (vx > 0) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
    } else if (abs_vy > abs_vx * 1.5f) {
        /* Vertical dominant */
        return (vy > 0) ? GESTURE_SWIPE_DOWN : GESTURE_SWIPE_UP;
    } else {
        /* Diagonal or ambiguous - classify as push/pull if distance changes */
        if (ca->present && ca->avg_distance > 0) {
            return GESTURE_PUSH; /* Simplified: treat as push */
        }
        return GESTURE_NONE;
    }
}

void gesture_engine_init(gesture_engine_t *engine,
                         VL53L5CX_Configuration *sensor_a,
                         VL53L5CX_Configuration *sensor_b)
{
    memset(engine, 0, sizeof(gesture_engine_t));
    engine->sensor_a = sensor_a;
    engine->sensor_b = sensor_b;
    engine->last_gesture.type = GESTURE_NONE;
}

int gesture_engine_sensors_init(gesture_engine_t *engine)
{
    uint8_t status;
    uint8_t is_alive;
    int retry;

    /* Check sensor A with retry (sensor may need boot time) */
    retry = 10;
    do {
        status = vl53l5cx_is_alive(engine->sensor_a, &is_alive);
        if (status == 0 && is_alive) break;
        board_delay_ms(10);
    } while (--retry > 0);
    if (status != 0 || !is_alive) {
        printf("VL53L5CX sensor A not found! (status=%d, alive=%d)\r\n", status, is_alive);
        return -1;
    }
    printf("Sensor A detected\r\n");

    /* Check sensor B with retry */
    retry = 10;
    do {
        status = vl53l5cx_is_alive(engine->sensor_b, &is_alive);
        if (status == 0 && is_alive) break;
        board_delay_ms(10);
    } while (--retry > 0);
    if (status != 0 || !is_alive) {
        printf("VL53L5CX sensor B not found! (status=%d, alive=%d)\r\n", status, is_alive);
        return -1;
    }
    printf("Sensor B detected\r\n");

    /* Init sensor A */
    status = vl53l5cx_init(engine->sensor_a);
    if (status != 0) {
        printf("VL53L5CX sensor A init failed (%d)!\r\n", status);
        return -1;
    }
    printf("Sensor A initialized\r\n");

    /* Init sensor B */
    status = vl53l5cx_init(engine->sensor_b);
    if (status != 0) {
        printf("VL53L5CX sensor B init failed (%d)!\r\n", status);
        return -1;
    }
    printf("Sensor B initialized\r\n");

    /* Configure both sensors: 8x8 @ 15Hz */
    vl53l5cx_set_resolution(engine->sensor_a, VL53L5CX_RESOLUTION_8X8);
    vl53l5cx_set_ranging_frequency_hz(engine->sensor_a, 15);
    vl53l5cx_set_target_order(engine->sensor_a, VL53L5CX_TARGET_ORDER_CLOSEST);

    vl53l5cx_set_resolution(engine->sensor_b, VL53L5CX_RESOLUTION_8X8);
    vl53l5cx_set_ranging_frequency_hz(engine->sensor_b, 15);
    vl53l5cx_set_target_order(engine->sensor_b, VL53L5CX_TARGET_ORDER_CLOSEST);

    /* Start ranging */
    vl53l5cx_start_ranging(engine->sensor_a);
    vl53l5cx_start_ranging(engine->sensor_b);

    printf("Both sensors started (8x8 @ 15Hz)\r\n");
    return 0;
}

void gesture_engine_capture_background(gesture_engine_t *engine)
{
    /* Wait for data ready and capture current frame as background */
    uint8_t ready = 0;
    int timeout = 100;

    /* Sensor A */
    do {
        if (vl53l5cx_check_data_ready(engine->sensor_a, &ready) != 0) break;
    } while (!ready && --timeout > 0);
    if (ready) {
        vl53l5cx_get_ranging_data(engine->sensor_a, &engine->results_a);
        for (int i = 0; i < GESTURE_ZONES_PER_SENSOR; i++) {
            engine->background_a[i] = engine->results_a.distance_mm[i];
        }
    }

    /* Sensor B */
    ready = 0;
    timeout = 100;
    do {
        if (vl53l5cx_check_data_ready(engine->sensor_b, &ready) != 0) break;
    } while (!ready && --timeout > 0);
    if (ready) {
        vl53l5cx_get_ranging_data(engine->sensor_b, &engine->results_b);
        for (int i = 0; i < GESTURE_ZONES_PER_SENSOR; i++) {
            engine->background_b[i] = engine->results_b.distance_mm[i];
        }
    }

    engine->background_valid = true;
    printf("Background captured\r\n");
}

bool gesture_engine_process(gesture_engine_t *engine)
{
    uint8_t ready_a = 0, ready_b = 0;
    centroid_t ca, cb;
    bool gesture_detected = false;

    /* Check data ready for both sensors */
    if (vl53l5cx_check_data_ready(engine->sensor_a, &ready_a) != 0) ready_a = 0;
    if (vl53l5cx_check_data_ready(engine->sensor_b, &ready_b) != 0) ready_b = 0;

    if (!ready_a && !ready_b) {
        return false;
    }

    /* Get ranging data */
    if (ready_a) {
        vl53l5cx_get_ranging_data(engine->sensor_a, &engine->results_a);
    }
    if (ready_b) {
        vl53l5cx_get_ranging_data(engine->sensor_b, &engine->results_b);
    }

    engine->frame_count++;

    /* Compute centroids */
    compute_centroid(engine->results_a.distance_mm,
                     engine->background_a, engine->background_valid, &ca);
    compute_centroid(engine->results_b.distance_mm,
                     engine->background_b, engine->background_valid, &cb);

    /* Update trackers */
    tracker_update(&engine->tracker_a, &ca);
    tracker_update(&engine->tracker_b, &cb);

    /* Compute velocity for sensor A (primary gesture sensor) */
    compute_velocity(&engine->tracker_a, &ca);

    /* Simple state machine for gesture detection */
    float speed = sqrtf(engine->tracker_a.velocity_x * engine->tracker_a.velocity_x +
                        engine->tracker_a.velocity_y * engine->tracker_a.velocity_y);

    switch (engine->tracker_a.state) {
    case GESTURE_STATE_IDLE:
        if (ca.present && speed > VELOCITY_THRESHOLD) {
            engine->tracker_a.state = GESTURE_STATE_TRACKING;
            engine->tracker_a.confirm_count = 1;
        } else if (ca.present && speed < HOVER_VELOCITY) {
            /* Potential hover - start counting */
            engine->tracker_a.confirm_count++;
            if (engine->tracker_a.confirm_count >= HOVER_FRAMES) {
                engine->last_gesture.type = GESTURE_HOVER;
                engine->last_gesture.cx = ca.cx;
                engine->last_gesture.cy = ca.cy;
                engine->last_gesture.avg_distance = ca.avg_distance;
                engine->last_gesture.speed = speed;
                gesture_detected = true;
                engine->tracker_a.confirm_count = 0;
            }
        } else {
            engine->tracker_a.confirm_count = 0;
        }
        break;

    case GESTURE_STATE_TRACKING:
        if (!ca.present) {
            /* Hand disappeared - gesture complete */
            gesture_type_t g = classify_gesture(
                engine->tracker_a.velocity_x, engine->tracker_a.velocity_y,
                &ca, &cb);
            if (g != GESTURE_NONE) {
                engine->last_gesture.type = g;
                engine->last_gesture.cx = ca.cx;
                engine->last_gesture.cy = ca.cy;
                engine->last_gesture.avg_distance = ca.avg_distance;
                engine->last_gesture.speed = speed;
                gesture_detected = true;
            }
            engine->tracker_a.state = GESTURE_STATE_IDLE;
            engine->tracker_a.confirm_count = 0;
        } else if (speed > VELOCITY_THRESHOLD) {
            engine->tracker_a.confirm_count++;
            if (engine->tracker_a.confirm_count >= CONFIRM_FRAMES) {
                /* Confirmed gesture direction - wait for end */
                engine->tracker_a.state = GESTURE_STATE_CONFIRMING;
            }
        } else {
            /* Velocity dropped - false alarm */
            engine->tracker_a.state = GESTURE_STATE_IDLE;
            engine->tracker_a.confirm_count = 0;
        }
        break;

    case GESTURE_STATE_CONFIRMING:
        if (!ca.present || speed < VELOCITY_THRESHOLD * 0.5f) {
            /* Gesture ending */
            gesture_type_t g = classify_gesture(
                engine->tracker_a.velocity_x, engine->tracker_a.velocity_y,
                &ca, &cb);
            if (g != GESTURE_NONE) {
                engine->last_gesture.type = g;
                engine->last_gesture.cx = ca.cx;
                engine->last_gesture.cy = ca.cy;
                engine->last_gesture.avg_distance = ca.avg_distance;
                engine->last_gesture.speed = speed;
                gesture_detected = true;
            }
            engine->tracker_a.state = GESTURE_STATE_IDLE;
            engine->tracker_a.confirm_count = 0;
        }
        break;
    }

    /* Store previous centroid */
    engine->tracker_a.prev_centroid = ca;
    engine->tracker_b.prev_centroid = cb;

    return gesture_detected;
}

const gesture_event_t *gesture_engine_get_last_gesture(const gesture_engine_t *engine)
{
    return &engine->last_gesture;
}

const centroid_t *gesture_engine_get_centroid_a(const gesture_engine_t *engine)
{
    return &engine->tracker_a.prev_centroid;
}

const centroid_t *gesture_engine_get_centroid_b(const gesture_engine_t *engine)
{
    return &engine->tracker_b.prev_centroid;
}

const int16_t *gesture_engine_get_distance_a(const gesture_engine_t *engine)
{
    return engine->results_a.distance_mm;
}

const int16_t *gesture_engine_get_distance_b(const gesture_engine_t *engine)
{
    return engine->results_b.distance_mm;
}

const char *gesture_type_str(gesture_type_t type)
{
    switch (type) {
    case GESTURE_NONE:        return "NONE";
    case GESTURE_SWIPE_LEFT:  return "SWIPE_LEFT";
    case GESTURE_SWIPE_RIGHT: return "SWIPE_RIGHT";
    case GESTURE_SWIPE_UP:    return "SWIPE_UP";
    case GESTURE_SWIPE_DOWN:  return "SWIPE_DOWN";
    case GESTURE_PUSH:        return "PUSH";
    case GESTURE_PULL:        return "PULL";
    case GESTURE_HOVER:       return "HOVER";
    default:                  return "UNKNOWN";
    }
}
