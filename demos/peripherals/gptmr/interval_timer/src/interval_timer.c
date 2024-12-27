/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "interval_timer.h"
#include "hpm_clock_drv.h"
#include "hpm_common.h"
#include "board.h"

#define GPTMR_CHANNEL_COUNT  4

typedef struct gptmr_clock_tag {
    GPTMR_Type *ptr;
    clock_name_t clock_name;
    uint16_t irq_num;
} gptmr_clock_tag_t;

const gptmr_clock_tag_t gptmr_clock_tags[] = {
#if defined(HPM_GPTMR0)
    {
        .ptr = HPM_GPTMR0,
        .clock_name = clock_gptmr0,
        .irq_num = IRQn_GPTMR0,
    },
#endif
#if defined(HPM_GPTMR1)
    {
        .ptr = HPM_GPTMR1,
        .clock_name = clock_gptmr1,
        .irq_num = IRQn_GPTMR1,
    },
#endif
#if defined(HPM_GPTMR2)
    {
        .ptr = HPM_GPTMR2,
        .clock_name = clock_gptmr2,
        .irq_num = IRQn_GPTMR2,
    },
#endif
#if defined(HPM_GPTMR3)
    {
        .ptr = HPM_GPTMR3,
        .clock_name = clock_gptmr3,
        .irq_num = IRQn_GPTMR3,
    },
#endif
#if defined(HPM_GPTMR4)
    {
        .ptr = HPM_GPTMR4,
        .clock_name = clock_gptmr4,
        .irq_num = IRQn_GPTMR4,
    },
#endif
#if defined(HPM_GPTMR5)
    {
        .ptr = HPM_GPTMR5,
        .clock_name = clock_gptmr5,
        .irq_num = IRQn_GPTMR5,
    },
#endif
#if defined(HPM_GPTMR6)
    {
        .ptr = HPM_GPTMR6,
        .clock_name = clock_gptmr6,
        .irq_num = IRQn_GPTMR6,
    },
#endif
#if defined(HPM_GPTMR7)
    {
        .ptr = HPM_GPTMR7,
        .clock_name = clock_gptmr7,
        .irq_num = IRQn_GPTMR7,
    },
#endif
#if defined(HPM_GPTMR8)
    {
        .ptr = HPM_GPTMR8,
        .clock_name = clock_gptmr8,
        .irq_num = IRQn_GPTMR8,
    },
#endif
#if defined(HPM_GPTMR9)
    {
        .ptr = HPM_GPTMR9,
        .clock_name = clock_gptmr9,
        .irq_num = IRQn_GPTMR9,
    },
#endif
};

void dma_channel_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)user_data;
    uint32_t dmamux_ch =  DMA_SOC_CHN_TO_DMAMUX_CHN(ptr, channel);
    dmamux_config(HPM_DMAMUX, dmamux_ch, *(uint32_t *)user_data, false);
}

void interval_timer_irq_handler(interval_timer_t *timer)
{
    if (gptmr_check_status(timer->ptr, GPTMR_CH_RLD_STAT_MASK(timer->channel))) {
        gptmr_clear_status(timer->ptr, GPTMR_CH_RLD_STAT_MASK(timer->channel));
        if (timer->status == timer_update) {
            gptmr_channel_config_update_reload(timer->ptr, timer->channel, timer->reload);
            gptmr_channel_update_count(timer->ptr, timer->channel, 0);
            timer->status = timer_begin; 
        }
        timer->cb();
    }
}

hpm_stat_t interval_timer_init(interval_timer_t *timer, GPTMR_Type *ptr, uint8_t channel)
{
    clock_name_t clock_name;
    gptmr_clock_tag_t *obj;
    uint8_t i;
    for (i = 0; i < (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t)); i++) {
        obj = (gptmr_clock_tag_t *)&gptmr_clock_tags[i];
        if (obj->ptr == ptr) {
            clock_name = obj->clock_name;
            break;
        }
    }
    if ((channel >= GPTMR_CHANNEL_COUNT) || (i >= (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t)))) {
        return status_invalid_argument;
    }
    clock_add_to_group(clock_name, BOARD_RUNNING_CORE & 0x1);
    timer->ptr = ptr;
    timer->channel = channel;
    timer->status = timer_end;
    return status_success;
}


hpm_stat_t interval_timer_start(interval_timer_t *timer, uint32_t microseconds, interval_timer_cb cb)
{
    uint32_t gptmr_freq;
    gptmr_clock_tag_t *obj;
    gptmr_channel_config_t config;
    clock_name_t clock_name;
    uint16_t irq_num;
    uint8_t i;
    for (i = 0; i < (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t)); i++) {
        obj = (gptmr_clock_tag_t *)&gptmr_clock_tags[i];
        if (obj->ptr == timer->ptr) {
            clock_name = obj->clock_name;
            irq_num = obj->irq_num;
            break;
        }
    }
    if ((i >= (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t))) || (timer->status != timer_end)) {
        return status_invalid_argument;
    }
    gptmr_channel_get_default_config(timer->ptr, &config);
    gptmr_freq = clock_get_frequency(clock_name);
    config.reload = gptmr_freq / 1000000 * microseconds;
    gptmr_channel_config(timer->ptr, timer->channel, &config, false);
    timer->cb = cb;
    timer->us = microseconds;
    timer->reload = config.reload;
    gptmr_enable_irq(timer->ptr, GPTMR_CH_RLD_IRQ_MASK(timer->channel));
    intc_m_enable_irq_with_priority(irq_num, 5);
    timer->status = timer_begin;  
    gptmr_start_counter(timer->ptr, timer->channel);
    gptmr_channel_reset_count(timer->ptr, timer->channel);
    return status_success;
}

hpm_stat_t interval_timer_priority(interval_timer_t *timer, uint8_t priority)
{
    gptmr_clock_tag_t *obj;
    uint16_t irq_num;
    uint8_t i;
    for (i = 0; i < (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t)); i++) {
        obj = (gptmr_clock_tag_t *)&gptmr_clock_tags[i];
        if (obj->ptr == timer->ptr) {
            irq_num = obj->irq_num;
            break;
        }
    }
    if (i >= (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t))) {
        return status_invalid_argument;
    }
    intc_m_enable_irq_with_priority(irq_num, priority);
    return status_success;
}


hpm_stat_t interval_timer_update(interval_timer_t *timer, uint32_t microseconds)
{
    gptmr_clock_tag_t *obj; 
    clock_name_t clock_name;
    uint32_t gptmr_freq;
    uint8_t i;
    for (i = 0; i < (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t)); i++) {
        obj = (gptmr_clock_tag_t *)&gptmr_clock_tags[i];
        if (obj->ptr == timer->ptr) {
            clock_name = obj->clock_name;
            break;
        }
    }
    if ((i >= (sizeof(gptmr_clock_tags) / sizeof(gptmr_clock_tag_t))) || (timer->status != timer_begin)) {
        return status_invalid_argument;
    }
    timer->us = microseconds;
    gptmr_freq = clock_get_frequency(clock_name);
    timer->reload = gptmr_freq / 1000000 * microseconds;
    timer->status = timer_update;  
    return status_success;
}

hpm_stat_t interval_timer_stop(interval_timer_t *timer)
{
    gptmr_stop_counter(timer->ptr, timer->channel);
    gptmr_channel_reset_count(timer->ptr, timer->channel);
    timer->status = timer_end;
    return status_success;
}


