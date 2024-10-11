/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_lobs_drv.h"
#include "cdc_acm.h"
#include "sampling.h"
#include "usb_config.h"

#define TRACE_CNT (20480)
#define TRIG_POINT 64

uint32_t lobs_buffer[TRACE_CNT * 4];
uint32_t report_buffer[TRACE_CNT];

extern int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len);

typedef struct pins_type
{
    uint32_t p0_d0 : 1;
    uint32_t p0_oe : 1;
    uint32_t p0_di : 1;

    uint32_t p1_d0 : 1;
    uint32_t p1_oe : 1;
    uint32_t p1_di : 1;

    uint32_t p2_d0 : 1;
    uint32_t p2_oe : 1;
    uint32_t p2_di : 1;

    uint32_t p3_d0 : 1;
    uint32_t p3_oe : 1;
    uint32_t p3_di : 1;

    uint32_t p4_d0 : 1;
    uint32_t p4_oe : 1;
    uint32_t p4_di : 1;

    uint32_t p5_d0 : 1;
    uint32_t p5_oe : 1;
    uint32_t p5_di : 1;

    uint32_t p6_d0 : 1;
    uint32_t p6_oe : 1;
    uint32_t p6_di : 1;

    uint32_t p7_d0 : 1;
    uint32_t p7_oe : 1;
    uint32_t p7_di : 1;

    uint32_t p8_d0 : 1;
    uint32_t p8_oe : 1;
    uint32_t p8_di : 1;

    uint32_t p9_d0 : 1;
    uint32_t p9_oe : 1;
    uint32_t p9_di : 1;

    uint32_t p10_d0 : 1;
    uint32_t p10_oe : 1;
    uint32_t p10_di : 1;

    uint32_t p11_d0 : 1;
    uint32_t p11_oe : 1;
    uint32_t p11_di : 1;

    uint32_t p12_d0 : 1;
    uint32_t p12_oe : 1;
    uint32_t p12_di : 1;

    uint32_t p13_d0 : 1;
    uint32_t p13_oe : 1;
    uint32_t p13_di : 1;

    uint32_t p14_d0 : 1;
    uint32_t p14_oe : 1;
    uint32_t p14_di : 1;

    uint32_t p15_d0 : 1;
    uint32_t p15_oe : 1;
    uint32_t p15_di : 1;

    uint32_t p16_d0 : 1;
    uint32_t p16_oe : 1;
    uint32_t p16_di : 1;

    uint32_t p17_d0 : 1;
    uint32_t p17_oe : 1;
    uint32_t p17_di : 1;

    uint32_t p18_d0 : 1;
    uint32_t p18_oe : 1;
    uint32_t p18_di : 1;

    uint32_t p19_d0 : 1;
    uint32_t p19_oe : 1;
    uint32_t p19_di : 1;

    uint32_t p20_d0 : 1;
    uint32_t p20_oe : 1;
    uint32_t p20_di : 1;

    uint32_t p21_d0 : 1;
    uint32_t p21_oe : 1;
    uint32_t p21_di : 1;

    uint32_t p22_d0 : 1;
    uint32_t p22_oe : 1;
    uint32_t p22_di : 1;

    uint32_t p23_d0 : 1;
    uint32_t p23_oe : 1;
    uint32_t p23_di : 1;

    uint32_t p24_d0 : 1;
    uint32_t p24_oe : 1;
    uint32_t p24_di : 1;

    uint32_t p25_d0 : 1;
    uint32_t p25_oe : 1;
    uint32_t p25_di : 1;

    uint32_t p26_d0 : 1;
    uint32_t p26_oe : 1;
    uint32_t p26_di : 1;

    uint32_t p27_d0 : 1;
    uint32_t p27_oe : 1;
    uint32_t p27_di : 1;

    uint32_t p28_d0 : 1;
    uint32_t p28_oe : 1;
    uint32_t p28_di : 1;

    uint32_t p29_d0 : 1;
    uint32_t p29_oe : 1;
    uint32_t p29_di : 1;

    uint32_t p30_d0 : 1;
    uint32_t p30_oe : 1;
    uint32_t p30_di : 1;

    uint32_t p31_d0 : 1;
    uint32_t p31_oe : 1;
    uint32_t p31_di : 1;

} pins_type_t;

typedef struct lobs_type
{
    uint32_t header_count;
    union
    {
        pins_type_t pins;
        uint32_t data[3];
    } buffer;
} lobs_type_t;

static void pins_convert_buffer(pins_type_t pins, uint32_t *buffer)
{
    (*buffer) = pins.p31_di << 31;
    (*buffer) |= pins.p30_di << 30;
    (*buffer) |= pins.p29_di << 29;
    (*buffer) |= pins.p28_di << 28;
    (*buffer) |= pins.p27_di << 27;
    (*buffer) |= pins.p26_di << 26;
    (*buffer) |= pins.p25_di << 25;
    (*buffer) |= pins.p24_di << 24;
    (*buffer) |= pins.p23_di << 23;
    (*buffer) |= pins.p22_di << 22;
    (*buffer) |= pins.p21_di << 21;
    (*buffer) |= pins.p20_di << 20;
    (*buffer) |= pins.p19_di << 19;
    (*buffer) |= pins.p18_di << 18;
    (*buffer) |= pins.p17_di << 17;
    (*buffer) |= pins.p16_di << 16;
    (*buffer) |= pins.p15_di << 15;
    (*buffer) |= pins.p14_di << 14;
    (*buffer) |= pins.p13_di << 13;
    (*buffer) |= pins.p12_di << 12;
    (*buffer) |= pins.p11_di << 11;
    (*buffer) |= pins.p10_di << 10;
    (*buffer) |= pins.p9_di << 9;
    (*buffer) |= pins.p8_di << 8;
    (*buffer) |= pins.p7_di << 7;
    (*buffer) |= pins.p6_di << 6;
    (*buffer) |= pins.p5_di << 5;
    (*buffer) |= pins.p4_di << 4;
    (*buffer) |= pins.p3_di << 3;
    (*buffer) |= pins.p2_di << 2;
    (*buffer) |= pins.p1_di << 1;
    (*buffer) |= pins.p0_di << 0;
}

void init_lobs_one_group_config(void)
{
    lobs_ctrl_config_t ctrl_config;
    lobs_state_config_t state_config = { 0 };

    lobs_unlock(HPM_LOBS);

    lobs_deinit(HPM_LOBS);

    ctrl_config.group_mode = lobs_one_group_128_bits;
    ctrl_config.sample_rate = lobs_sample_1_per_7;
    ctrl_config.start_addr = (uint32_t)&lobs_buffer[0];
    ctrl_config.end_addr = (uint32_t)&lobs_buffer[0] + sizeof(lobs_buffer);
    lobs_ctrl_config(HPM_LOBS, &ctrl_config);

    state_config.sig_group_num = 5;
    state_config.cmp_mode = lobs_cnt_cmp_mode;
    state_config.state_chg_condition = lobs_cnt_matched;
    state_config.next_state = lobs_next_state_finish;
    state_config.cmp_counter = (TRACE_CNT);
    state_config.cmp_sig_en[0] = false;
    state_config.cmp_sig_en[1] = false;
    state_config.cmp_sig_en[2] = false;
    state_config.cmp_sig_en[3] = false;
    lobs_state_config(HPM_LOBS, lobs_state_0, &state_config);


    lobs_lock(HPM_LOBS);
}

void lobs_open(void)
{
    lobs_unlock(HPM_LOBS);
    lobs_set_pre_trig_enable(HPM_LOBS, true);
    lobs_set_state_enable(HPM_LOBS, lobs_state_0, true);
    lobs_set_enable(HPM_LOBS, true);
    lobs_lock(HPM_LOBS);
}

void lobs_close(void)
{
    lobs_unlock(HPM_LOBS);
    lobs_set_pre_trig_enable(HPM_LOBS, false);
    lobs_set_state_enable(HPM_LOBS, lobs_state_0, false);
    lobs_set_enable(HPM_LOBS, false);
    lobs_lock(HPM_LOBS);
}

void lobs_loop(void)
{
    lobs_type_t *buffer = NULL;
    if (lobs_is_trace_finish(HPM_LOBS) == true) {
        lobs_close();
        printf("trace memory base addr: %#x, trace data final addr: %#x\n\n", (uint32_t)lobs_buffer, lobs_get_final_address(HPM_LOBS));
        for (size_t i = 0; i < TRACE_CNT; i++) {
            buffer = (lobs_type_t *)&lobs_buffer[(i * 4)];
            pins_convert_buffer(buffer->buffer.pins, &report_buffer[i]);
        }
        if (!get_usb_cdc_tx_busy()) {
            set_usb_cdc_tx_busy(true);
            usbd_ep_start_write(0, CDC_IN_EP, (uint8_t*)report_buffer, sizeof(report_buffer));
            while (get_usb_cdc_tx_busy()) {
            };
        }
        lobs_open();
    }
}

