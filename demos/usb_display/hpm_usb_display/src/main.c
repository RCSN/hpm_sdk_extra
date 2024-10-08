/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_debug_console.h"
#include "usb_config.h"

#define LED_FLASH_PERIOD_IN_MS 300

extern void usb_display_init(void);

int main(void)
{
    board_init();
    board_init_led_pins();
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);

    // board_timer_create(LED_FLASH_PERIOD_IN_MS, board_led_toggle);

    printf("cherry usb display device sample.\n");

    usb_display_init();

    while (1) {
    }
    return 0;
}
