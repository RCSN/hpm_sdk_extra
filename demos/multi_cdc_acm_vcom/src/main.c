/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "cdc_acm.h"
int main(void)
{
    uint8_t i = 0;
    uint8_t buffer[100];
    uint8_t len = 0;
    board_init();

    printf("cherryusb multi cdc_acm device sample.\n");

    board_init_usb_pins();
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 3);
    cdc_acm_init(USB_BUS_ID, CONFIG_HPM_USBD_BASE);

    while (1) {
        for (i = 0; i < MAX_CDC_COUNT; i++) {
            if (cdc_device[i].is_open == true) {
                cdc_device[i].is_open = false;
                len = sprintf((char *)buffer, "[system]cdc_vcom number:%d\r\n", cdc_device[i].uart_num);
                usbd_ep_start_write(USB_BUS_ID, cdc_device[i].cdc_in_ep.ep_addr, buffer, len);
            }
        }
        board_delay_ms(100);
    }
    return 0;
}
