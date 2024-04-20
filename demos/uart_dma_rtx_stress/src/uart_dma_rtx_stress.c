/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_uart_drv.h"
#include "hpm_debug_console.h"
#include "hpm_dma_mgr.h"
#include "hpm_sysctl_drv.h"
#include "hpm_gptmr_drv.h"
#include "chry_ringbuffer.h"

#define UART_BAUDRATE                 (6000000U)
#define UART_BASE                     BOARD_APP_UART_BASE
#define UART_CLK_NAME                 BOARD_APP_UART_CLK_NAME
#define UART_RX_DMA                   BOARD_APP_UART_RX_DMA_REQ
#define UART_RX_DMA_RESOURCE_INDEX    (0U)
#define UART_RX_DMA_BUFFER_SIZE       (4096U)

#define UART_TX_DMA                   BOARD_APP_UART_TX_DMA_REQ
#define UART_TX_DMA_RESOURCE_INDEX    (1U)
#define UART_TX_DMA_BUFFER_SIZE       (4096U)

#define APP_TICK_MS                   (1)
#define APP_BOARD_GPTMR               BOARD_GPTMR
#define APP_BOARD_GPTMR_CH            BOARD_GPTMR_CHANNEL
#define APP_BOARD_GPTMR_IRQ           BOARD_GPTMR_IRQ
#define APP_BOARD_GPTMR_CLOCK         BOARD_GPTMR_CLK_NAME

static dma_resource_t dma_resource_pools[2];
static bool tx_dma_complete_flag = false;
static uint32_t rx_front_index;
static uint32_t rx_rear_index;
chry_ringbuffer_t rb;

ATTR_PLACE_AT_NONCACHEABLE                       uint8_t rb_mempool[UART_RX_DMA_BUFFER_SIZE * 8];
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4) uint8_t uart_tx_buf[UART_TX_DMA_BUFFER_SIZE];
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4) uint8_t uart_rx_buf[UART_RX_DMA_BUFFER_SIZE];
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t rx_descriptors[2];

static void board_uart_init_config(void);
static void board_uart_dma_config(void);
static void timer_config(void);
static void task_entry_ms(void);

void tick_ms_isr(void)
{
    if (gptmr_check_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH))) {
        gptmr_clear_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH));
        task_entry_ms();
    }
}
SDK_DECLARE_EXT_ISR_M(APP_BOARD_GPTMR_IRQ, tick_ms_isr);

void dma_channel_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)channel;
    *(volatile bool *)user_data = true;
}

int main(void)
{
    uint32_t tx_size;
    uint32_t rb_tsize;
    uint32_t len;
    uint32_t buf_addr;
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    board_init();
    dma_mgr_init();
    timer_config();
    board_uart_init_config();
    board_uart_dma_config();
    if (0 == chry_ringbuffer_init(&rb, rb_mempool, sizeof(rb_mempool))) {
        printf("chry_ringbuffer_init success\r\n");
    } else {
        printf("chry_ringbuffer_init error\r\n");
        while (1) {
        }
    }
    while (1) {
        rb_tsize = chry_ringbuffer_get_used(&rb);
        tx_size = (rb_tsize >= UART_TX_DMA_BUFFER_SIZE) ? UART_TX_DMA_BUFFER_SIZE : rb_tsize;
        len = chry_ringbuffer_read(&rb, uart_tx_buf, tx_size);
        if (len > 0) {
            buf_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)uart_tx_buf);
            dma_mgr_set_chn_src_addr(tx_resource, buf_addr);
            dma_mgr_set_chn_transize(tx_resource, len);
            dma_mgr_enable_channel(tx_resource);
            while (tx_dma_complete_flag == false) {
            }
            tx_dma_complete_flag = false;
        }
    }
}

static void board_uart_init_config(void)
{
    uart_config_t config = {0};
    board_init_uart(UART_BASE);
    clock_set_source_divider(UART_CLK_NAME, clk_src_pll1_clk1, 4);
    uart_default_config(UART_BASE, &config);
    config.baudrate = UART_BAUDRATE;
    config.fifo_enable = true;
    config.dma_enable = true;
    config.src_freq_in_hz = clock_get_frequency(UART_CLK_NAME);
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    if (uart_init(UART_BASE, &config) != status_success) {
        printf("failed to initialize uart\n");
        while (1) {
        }
    }
}

static void board_uart_dma_config(void)
{
    dma_mgr_chn_conf_t chg_config;
    dma_resource_t *resource = NULL;
    dma_mgr_get_default_chn_config(&chg_config);
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    /* uart rx dma config */
    resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success) {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.src_addr = (uint32_t)&UART_BASE->RBR;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.size_in_byte = UART_RX_DMA_BUFFER_SIZE;
        chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf);
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_RX_DMA;
        chg_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[1]);
        if (dma_mgr_config_linked_descriptor(resource, &chg_config, (dma_mgr_linked_descriptor_t *)&rx_descriptors[0]) != status_success) {
            while (1) {
            }
        }
        chg_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
        if (dma_mgr_config_linked_descriptor(resource, &chg_config, (dma_mgr_linked_descriptor_t *)&rx_descriptors[1]) != status_success) {
            while (1) {
            }
        }
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_enable_channel(resource);
    }
     /* uart tx dma config */
    resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success) {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.dst_addr =(uint32_t)&UART_BASE->THR;
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_TX_DMA;
        chg_config.linked_ptr = NULL;
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, (void *)&tx_dma_complete_flag);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
    }
}

static void timer_config(void)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(APP_BOARD_GPTMR, &config);

    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    config.reload = gptmr_freq / 1000 * APP_TICK_MS;
    gptmr_channel_config(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH, &config, false);
    gptmr_start_counter(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);

    gptmr_enable_irq(APP_BOARD_GPTMR, GPTMR_CH_RLD_IRQ_MASK(APP_BOARD_GPTMR_CH));
    intc_m_enable_irq_with_priority(APP_BOARD_GPTMR_IRQ, 1);
}

static void task_entry_ms(void)
{
    uint32_t rx_data_size;
    uint32_t dma_remaining_size;
    dma_resource_t *resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    dma_remaining_size = dma_get_remaining_transfer_size(resource->base, resource->channel);

    rx_rear_index = UART_RX_DMA_BUFFER_SIZE - dma_remaining_size;
    if (rx_front_index > rx_rear_index) {
        rx_data_size = UART_RX_DMA_BUFFER_SIZE + rx_rear_index - rx_front_index;
    } else {
        rx_data_size = rx_rear_index - rx_front_index;
    }

    for (uint32_t i = 0; i < rx_data_size; i++) {
        if ((rx_front_index + i) < UART_RX_DMA_BUFFER_SIZE) {
            chry_ringbuffer_write_byte(&rb, uart_rx_buf[rx_front_index + i]);
        } else {
            chry_ringbuffer_write_byte(&rb, uart_rx_buf[rx_front_index + i - UART_RX_DMA_BUFFER_SIZE]);
        }
    }
    rx_front_index = rx_rear_index;
}

