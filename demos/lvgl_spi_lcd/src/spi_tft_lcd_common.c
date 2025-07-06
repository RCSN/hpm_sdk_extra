/*
* Copyright (c) 2025 HPMicro
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#include "board.h"
#include "spi_tft_lcd_common.h"

static void spi_tft_lcd_gpio_init(spi_tft_lcd_context_t *ctx);
static hpm_stat_t spi_tft_lcd_spi_init(spi_tft_lcd_context_t *ctx);

hpm_stat_t spi_tft_lcd_hardware_init(spi_tft_lcd_context_t *ctx)
{
    hpm_stat_t stat;
    spi_tft_lcd_gpio_init(ctx);
    stat = spi_tft_lcd_spi_init(ctx);
    return stat;
}

void spi_tft_lcd_set_dc(spi_tft_lcd_context_t *ctx, bool state)
{
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->dc_pin), GPIO_GET_PIN_INDEX(ctx->dc_pin), state);
}

void spi_tft_lcd_set_rst(spi_tft_lcd_context_t *ctx, bool state)
{
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->rst_pin), GPIO_GET_PIN_INDEX(ctx->rst_pin), state);
}

void spi_tft_lcd_set_bl(spi_tft_lcd_context_t *ctx, bool state)
{
    if (ctx->use_bl == false) {
        (void)state;
        return;
    }
    if (ctx->bl_level == 0) {
        state = !state;
    }
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->bl_pin), GPIO_GET_PIN_INDEX(ctx->bl_pin), state);
}

void spi_tft_lcd_set_te_interrupt(spi_tft_lcd_context_t *ctx, bool enable)
{
    if (ctx->use_te == false) {
        (void)enable;
        return;
    }
    gpio_config_pin_interrupt(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin), ctx->te_trigger);
    if (enable == false) {
        gpio_disable_pin_interrupt(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin));
    } else {
        gpio_enable_pin_interrupt(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin));
    }
    intc_m_enable_irq_with_priority(ctx->te_irq, ctx->te_priority);
}


void spi_tft_lcd_clear_te_interrupt_flag(spi_tft_lcd_context_t *ctx)
{
    if (ctx->use_te == false) {
        return;
    }
    gpio_clear_pin_interrupt_flag(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin));
}

bool spi_tft_lcd_te_get_state(spi_tft_lcd_context_t *ctx)
{
    if (ctx->use_te == false) {
        return false;
    }
    return gpio_read_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin));
}

void spi_tft_lcd_set_cs(spi_tft_lcd_context_t *ctx, bool state)
{
    if (ctx->use_soft_cs == false) {
        (void)state;
        return;
    }
    if (ctx->cs_level == 0) {
        state =!state;
    }
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->cs_pin), GPIO_GET_PIN_INDEX(ctx->cs_pin), state);
}

void spi_tft_lcd_set_lvgl_test_tx_pin_toggle(spi_tft_lcd_context_t *ctx)
{
    if (ctx->use_lvgl_test_tx == false) {
        return;
    }
    gpio_toggle_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_test_tx_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_test_tx_pin));
}

void spi_tft_lcd_set_lvgl_test_tx_pin(spi_tft_lcd_context_t *ctx, bool state)
{
    if (ctx->use_lvgl_test_tx == false) {
        return;
    }
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_test_tx_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_test_tx_pin), state);
}

void spi_tft_lcd_set_lvgl_test_refr_timer_pin(spi_tft_lcd_context_t *ctx, bool state)
{
    if (ctx->use_lvgl_refr_timer == false) {
        return;
    }
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_refr_timer_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_refr_timer_pin), state);
}

hpm_stat_t spi_tft_lcd_transfer_data_blocking(spi_tft_lcd_context_t *ctx, uint8_t bit_width, uint8_t *data, uint32_t size, uint32_t timeout)
{
    hpm_stat_t stat = status_success;
    if (bit_width <= 0 || bit_width > 32) {
        return status_invalid_argument;
    }
    spi_set_data_bits(ctx->spi_base, bit_width);
    stat = hpm_spi_transmit_blocking(ctx->spi_base, data, size, timeout);
    return stat;
}

hpm_stat_t spi_tft_lcd_transfer_data_nonblocking(spi_tft_lcd_context_t *ctx, uint8_t bit_width, uint8_t *data, uint32_t size)
{
    hpm_stat_t stat = status_success;
    if (bit_width <= 0 || bit_width > 32) {
        return status_invalid_argument;
    }
    spi_set_data_bits(ctx->spi_base, bit_width);
    stat = hpm_spi_transmit_nonblocking(ctx->spi_base, data, size);
    return stat;
}

static hpm_stat_t spi_tft_lcd_spi_init(spi_tft_lcd_context_t *ctx)
{
    hpm_stat_t stat;
    spi_initialize_config_t init_config;
    hpm_spi_get_default_init_config(&init_config);
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_even_clk_edges;
    init_config.clk_polarity = spi_sclk_high_idle;
    init_config.data_len = 8U;
    /* step.1  initialize spi */
    stat = hpm_spi_initialize(ctx->spi_base, &init_config);
    if (stat != status_success) {
        printf("hpm_spi_initialize fail\n");
        return stat;
    }
    /* step.2  set spi sclk frequency for master */
    stat = hpm_spi_set_sclk_frequency(ctx->spi_base, ctx->spi_sclk_freq);
    if (stat != status_success) {
        printf("hpm_spi_set_sclk_frequency fail\n");
        return stat;
    }
    /* step.3 install dma callback if want use dma */
    stat = hpm_spi_dma_mgr_install_callback(ctx->spi_base, ctx->tx_complete, NULL);
    if (stat != status_success) {
        printf("hpm_spi_dma_mgr_install_callback fail\n");
        return stat;
    }
    return stat;
}

static void spi_tft_lcd_gpio_init(spi_tft_lcd_context_t *ctx)
{
    /* pinmux init DC pin */
    HPM_IOC->PAD[ctx->dc_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
    if (ctx->dc_pin >= IOC_PAD_PZ00) {
        /* PZ port IO needs to configure BIOC as well */
        HPM_BIOC->PAD[ctx->dc_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
    }
#endif
#if defined(IOC_PAD_PY00)
    else if (ctx->dc_pin >= IOC_PAD_PY00) {
        HPM_PIOC->PAD[ctx->dc_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
    }
#endif
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->dc_pin), GPIO_GET_PIN_INDEX(ctx->dc_pin));

    /* pinmux init RESET pin */
    HPM_IOC->PAD[ctx->rst_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
    if (ctx->rst_pin >= IOC_PAD_PZ00) {
        /* PZ port IO needs to configure BIOC as well */
        HPM_BIOC->PAD[ctx->rst_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
    }
#endif
#if defined(IOC_PAD_PY00)
    else if (ctx->rst_pin >= IOC_PAD_PY00) {
        HPM_PIOC->PAD[ctx->rst_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
    }
#endif
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->rst_pin), GPIO_GET_PIN_INDEX(ctx->rst_pin));

    /* pinmux init BL pin */
    if (ctx->use_bl == true) {
        HPM_IOC->PAD[ctx->bl_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
        if (ctx->bl_pin >= IOC_PAD_PZ00) {
            /* PZ port IO needs to configure BIOC as well */
            HPM_BIOC->PAD[ctx->bl_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
#if defined(IOC_PAD_PY00)
        else if (ctx->bl_pin >= IOC_PAD_PY00) {
            HPM_PIOC->PAD[ctx->bl_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->bl_pin), GPIO_GET_PIN_INDEX(ctx->bl_pin));    
    }

    /* pinmux init TE pin */
    if (ctx->use_te == true) {
        HPM_IOC->PAD[ctx->te_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
        if (ctx->te_pin >= IOC_PAD_PZ00) {
            /* PZ port IO needs to configure BIOC as well */
            HPM_BIOC->PAD[ctx->te_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
#if defined(IOC_PAD_PY00)
        else if (ctx->te_pin >= IOC_PAD_PY00) {
            HPM_PIOC->PAD[ctx->te_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
        gpio_set_pin_input(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->te_pin), GPIO_GET_PIN_INDEX(ctx->te_pin));
    }

    /* pinmux init test_tx pin*/
    if (ctx->use_lvgl_test_tx == true) {
        HPM_IOC->PAD[ctx->lvgl_test_tx_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
        if (ctx->lvgl_test_tx_pin >= IOC_PAD_PZ00) {
            /* PZ port IO needs to configure BIOC as well */
            HPM_BIOC->PAD[ctx->lvgl_test_tx_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
#if defined(IOC_PAD_PY00)
        else if (ctx->lvgl_test_tx_pin >= IOC_PAD_PY00) {
            HPM_PIOC->PAD[ctx->lvgl_test_tx_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_test_tx_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_test_tx_pin));
        gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_test_tx_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_test_tx_pin), 0);
    }

    /* pinmux init lvgl refr timer pin */
    if (ctx->use_lvgl_refr_timer == true) {
        HPM_IOC->PAD[ctx->lvgl_refr_timer_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
        if (ctx->lvgl_refr_timer_pin >= IOC_PAD_PZ00) {
            /* PZ port IO needs to configure BIOC as well */
            HPM_BIOC->PAD[ctx->lvgl_refr_timer_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
#if defined(IOC_PAD_PY00)
        else if (ctx->lvgl_refr_timer_pin >= IOC_PAD_PY00) {
            HPM_PIOC->PAD[ctx->lvgl_refr_timer_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_refr_timer_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_refr_timer_pin));
        gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->lvgl_refr_timer_pin), GPIO_GET_PIN_INDEX(ctx->lvgl_refr_timer_pin), 0);
    }

    board_init_spi_clock(ctx->spi_base);
    board_init_spi_pins(ctx->spi_base);

    /* pinmux init CS pin */
    if (ctx->use_soft_cs == true) {
        HPM_IOC->PAD[ctx->cs_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
#if defined(IOC_PAD_PZ00)
        if (ctx->cs_pin >= IOC_PAD_PZ00) {
            /* PZ port IO needs to configure BIOC as well */
            HPM_BIOC->PAD[ctx->cs_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
#if defined(IOC_PAD_PY00)
        else if (ctx->cs_pin >= IOC_PAD_PY00) {
            HPM_PIOC->PAD[ctx->cs_pin].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(3);
        }
#endif
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->cs_pin), GPIO_GET_PIN_INDEX(ctx->cs_pin));
        gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(ctx->cs_pin), GPIO_GET_PIN_INDEX(ctx->cs_pin), !ctx->cs_level);
    }
}