/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "WS2812.h"
#include <hpm_dma_mgr.h>
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "cdc_acm.h"
#include "slcan.h"
#include "mcan.h"
int main(void)
{
    // uint8_t i = 0;
    // uint8_t buffer[100];
    // uint8_t len = 0;
    board_init();
    dma_mgr_init();
    WS2812_Init();
    printf("cherryusb multi cdc_acm device sample.\n");

    board_init_usb(HPM_USB0);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 3);
    cdc_acm_init(USB_BUS_ID, CONFIG_HPM_USBD_BASE);
    slcan_port0_init(&slcan0);
    slcan_port1_init(&slcan1);
    slcan_port2_init(&slcan2);
    slcan_port3_init(&slcan3);
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0x0B, 0, 0);
    }
    WS2812_Update(true);
    board_delay_ms(100);

    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0, 0x0B, 0);
        
    }
    WS2812_Update(true);
    board_delay_ms(100);

    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0, 0, 0x0B);
    }
    WS2812_Update(true);
    board_delay_ms(100);

    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetPixel(i, 0, 0, 0);
    }
    WS2812_Update(true);
    while (1) {
        slcan_process_task(&slcan0);
        slcan_process_task(&slcan1);
        slcan_process_task(&slcan2);
        slcan_process_task(&slcan3);
    }
    return 0;
}
