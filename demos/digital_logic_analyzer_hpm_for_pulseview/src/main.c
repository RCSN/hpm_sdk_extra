/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_debug_console.h"
#include "usb_config.h"
#include "sampling.h"
#include "hpm_gptmr_drv.h"
#include "hpm_l1c_drv.h"

#define APP_BOARD_PWM                 BOARD_GPTMR_PWM
#define APP_BOARD_PWM_CH              BOARD_GPTMR_PWM_CHANNEL
#define APP_BOARD_GPTMR_CLOCK         BOARD_GPTMR_PWM_CLK_NAME

#define APP_PWM_MAX_FREQ                  (100000U)
#define APP_PWM_MIN_FREQ                  (10000U)
#define APP_PWM_MAX_DUTY                  (100U)
#define APP_PWM_MAX_DUTY_STEP             (2U)

#define LED_FLASH_PERIOD_IN_MS 300

/**
 * @brief set waveform edge aligned pwm frequency
 *
 * @param [in] freq pwm frequency, the unit is HZ
 */

static void set_pwm_waveform_edge_aligned_frequency(uint32_t freq);

/**
 * @brief set waveform edge aligned pwm duty
 *
 * @param [in] duty pwm duty, the value range is 1 to 100
 */

static void set_pwm_waveform_edge_aligned_duty(uint8_t duty);

extern volatile bool dtr_enable;
extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
extern void cdc_acm_data_send_with_dtr_test(uint8_t busid);
uint32_t current_reload;
int main(void)
{
    l1c_dc_disable();
    board_init();
    init_gptmr_pins(APP_BOARD_PWM);
    printf("pwm generate testing\n");
    set_pwm_waveform_edge_aligned_frequency(10000000);
    set_pwm_waveform_edge_aligned_duty(50);
    board_init_led_pins();
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);

    // board_timer_create(LED_FLASH_PERIOD_IN_MS, board_led_toggle);

    printf("digital logic analyzer hpm6e00 for pulseview.\n");

    cdc_acm_init(0, CONFIG_HPM_USBD_BASE);
    init_lobs_one_group_config();

    while (1) {
        lobs_loop();
    }
    return 0;
}

static void set_pwm_waveform_edge_aligned_frequency(uint32_t freq)
{
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;

    gptmr_channel_get_default_config(APP_BOARD_PWM, &config);
    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    current_reload = gptmr_freq / freq;
    config.reload = current_reload;
    config.cmp_initial_polarity_high = false;
    gptmr_stop_counter(APP_BOARD_PWM, APP_BOARD_PWM_CH);
    gptmr_channel_config(APP_BOARD_PWM, APP_BOARD_PWM_CH, &config, false);
    gptmr_channel_reset_count(APP_BOARD_PWM, APP_BOARD_PWM_CH);
    gptmr_start_counter(APP_BOARD_PWM, APP_BOARD_PWM_CH);
}

static void set_pwm_waveform_edge_aligned_duty(uint8_t duty)
{
    uint32_t cmp;
    if (duty > 100) {
        duty = 100;
    }
    cmp = (current_reload * duty) / 100;
    gptmr_update_cmp(APP_BOARD_PWM, APP_BOARD_PWM_CH, 0, cmp);
    gptmr_update_cmp(APP_BOARD_PWM, APP_BOARD_PWM_CH, 1, current_reload);
}


