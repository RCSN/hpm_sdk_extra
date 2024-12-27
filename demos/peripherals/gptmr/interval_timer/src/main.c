/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_sysctl_drv.h"
#include "hpm_gpio_drv.h"
#include "interval_timer.h"

#define APP_BOARD_GPTMR               BOARD_GPTMR
#define APP_BOARD_GPTMR_CH            BOARD_GPTMR_CHANNEL
#define APP_BOARD_GPTMR_IRQ           BOARD_GPTMR_IRQ

SDK_DECLARE_EXT_ISR_M(APP_BOARD_GPTMR_IRQ, gptmr_irq)

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) interval_timer_t my_timer;
volatile uint32_t timer_count = 0;

void gptmr_irq(void)
{
    interval_timer_irq_handler(&my_timer);
}

void io_toggle_cb(void)
{
    gpio_toggle_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(IOC_PAD_PA31), GPIO_GET_PIN_INDEX(IOC_PAD_PA31));
    timer_count++;
}

int main(void)
{
    board_init();
    HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PA31_FUNC_CTL_GPIO_A_31;
    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_GET_PORT_INDEX(IOC_PAD_PA31), GPIO_GET_PIN_INDEX(IOC_PAD_PA31), 0);
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(IOC_PAD_PA31), GPIO_GET_PIN_INDEX(IOC_PAD_PA31), 1);
    interval_timer_init(&my_timer, APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
    interval_timer_start(&my_timer, 10, io_toggle_cb); /* 10us */
    while (1) {
        if (timer_count == 50) {
            interval_timer_update(&my_timer, 20);
        } else if (timer_count == 100) {
            interval_timer_stop(&my_timer);
        }        
    }
}

