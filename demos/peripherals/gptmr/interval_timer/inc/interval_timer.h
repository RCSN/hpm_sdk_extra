/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef HPM_INTERVAL_TIMER_H
#define HPM_INTERVAL_TIMER_H

#include "hpm_gptmr_drv.h"

typedef void (*interval_timer_cb)(void);

typedef enum interval_timer_status {
    timer_begin = 0,
    timer_update,
    timer_end,
} interval_timer_status_t;

typedef struct interval_timer_config {
    GPTMR_Type *ptr;
    uint8_t channel;
    interval_timer_status_t status;
    uint32_t us;
    uint32_t reload;
    interval_timer_cb cb;
} interval_timer_t;

#ifdef __cplusplus
extern "C" {
#endif
void interval_timer_irq_handler(interval_timer_t *timer);
hpm_stat_t interval_timer_init(interval_timer_t *timer, GPTMR_Type *ptr, uint8_t channel);
hpm_stat_t interval_timer_start(interval_timer_t *timer, uint32_t microseconds, interval_timer_cb cb);
hpm_stat_t interval_timer_priority(interval_timer_t *timer, uint8_t priority);
hpm_stat_t interval_timer_update(interval_timer_t *timer, uint32_t microseconds);
hpm_stat_t interval_timer_stop(interval_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif
