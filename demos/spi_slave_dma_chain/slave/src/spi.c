/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_spi_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_l1c_drv.h"

#define TEST_SPI               BOARD_APP_SPI_BASE
#define TEST_SPI_DMA           BOARD_APP_HDMA
#define TEST_SPI_DMAMUX        BOARD_APP_DMAMUX
#define TEST_SPI_RX_DMA_REQ    BOARD_APP_SPI_RX_DMA
#define TEST_SPI_TX_DMA_REQ    BOARD_APP_SPI_TX_DMA
#define TEST_SPI_RX_DMA_CH     0
#define TEST_SPI_TX_DMA_CH     1
#define TEST_SPI_RX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(TEST_SPI_DMA, TEST_SPI_RX_DMA_CH)
#define TEST_SPI_TX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(TEST_SPI_DMA, TEST_SPI_TX_DMA_CH)
#define TEST_SPI_IRQ       BOARD_APP_SPI_IRQ
#define TEST_SPI_DMA_IRQ     BOARD_APP_HDMA_IRQ
/* data width definition */
#define TEST_SPI_DATA_LEN_IN_BIT          (8U)
#define TEST_SPI_DATA_LEN_IN_BYTE         (1U)
#define TEST_SPI_DMA_TRANS_DATA_WIDTH     DMA_TRANSFER_WIDTH_BYTE

#ifndef PLACE_BUFF_AT_CACHEABLE
#define PLACE_BUFF_AT_CACHEABLE 1
#endif

#define TEST_TRANSFER_DATA_IN_BYTE         (260U)
#define TEST_TRANSFER_DMA_TRANS_CONT       (20U)
/* descriptor should be 8-byte aligned */
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t rx_descriptors[TEST_TRANSFER_DMA_TRANS_CONT];

/* dma buffer should be 4-byte aligned */
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4) uint8_t receive_buff_trans[TEST_TRANSFER_DMA_TRANS_CONT + 1][TEST_TRANSFER_DATA_IN_BYTE];

ATTR_PLACE_AT_NONCACHEABLE uint8_t send_dummy = 0;
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t tx_descriptors[2];


volatile bool spi_transfer_done;
volatile bool spi_rx_dma_trans_done;
volatile bool spi_tx_dma_trans_done;
volatile static uint8_t rx_count = 0;

const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static hpm_stat_t spi_rx_trigger_dma(void)
{

    hpm_stat_t stat;
    dma_channel_config_t rx_ch_config = { 0 };

    dmamux_config(TEST_SPI_DMAMUX, TEST_SPI_RX_DMAMUX_CH, TEST_SPI_RX_DMA_REQ, true);
    dma_default_channel_config(TEST_SPI_DMA, &rx_ch_config);
    rx_ch_config.src_addr = (uint32_t)&TEST_SPI->DATA;
    rx_ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    rx_ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    rx_ch_config.src_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
    rx_ch_config.src_width = TEST_SPI_DMA_TRANS_DATA_WIDTH;

    rx_ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
    rx_ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
    rx_ch_config.dst_width = TEST_SPI_DMA_TRANS_DATA_WIDTH;

    rx_ch_config.size_in_byte = TEST_TRANSFER_DATA_IN_BYTE;

    for (uint16_t i = 0; i < TEST_TRANSFER_DMA_TRANS_CONT; i++) {
        rx_ch_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)receive_buff_trans[i + 1]);
        if (i == (TEST_TRANSFER_DMA_TRANS_CONT - 1)) {
            rx_ch_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
        } else {
            rx_ch_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[i + 1]);
        }
        stat = dma_config_linked_descriptor(TEST_SPI_DMA, &rx_descriptors[i], TEST_SPI_RX_DMA_CH, &rx_ch_config);
    }
    rx_ch_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)receive_buff_trans[0]);
    rx_ch_config.linked_ptr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&rx_descriptors[0]);
    stat = dma_setup_channel(TEST_SPI_DMA, TEST_SPI_RX_DMA_CH, &rx_ch_config, true);
    if (stat != status_success) {
        printf("spi_rx_trigger_dma init fail\n");
        while (1) {
        };
    }
    return stat;
}

void isr_dma(void)
{
    volatile hpm_stat_t rx_stat, tx_stat;
      rx_stat = dma_check_transfer_status(TEST_SPI_DMA, TEST_SPI_RX_DMA_CH);
    tx_stat = dma_check_transfer_status(TEST_SPI_DMA, TEST_SPI_TX_DMA_CH);
    if (rx_stat & (DMA_CHANNEL_STATUS_TC | DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) {
        spi_rx_trigger_dma();
        rx_count = 0;
    }
    if (tx_stat & (DMA_CHANNEL_STATUS_TC | DMA_CHANNEL_STATUS_ERROR | DMA_CHANNEL_STATUS_ABORT)) {
        spi_tx_trigger_dma();
    }
}
SDK_DECLARE_EXT_ISR_M(TEST_SPI_DMA_IRQ, isr_dma)

void spi_isr(void)
{
    volatile uint32_t irq_status;

    irq_status = spi_get_interrupt_status(TEST_SPI); /* get interrupt stat */

    if (irq_status & spi_end_int) {
        spi_transfer_done = true;
        spi_clear_interrupt_status(TEST_SPI, spi_end_int);
    }
}
SDK_DECLARE_EXT_ISR_M(TEST_SPI_IRQ, spi_isr)


uint16_t crc16_byte(uint16_t crc, uint8_t data)
{
	return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

uint16_t crc16(uint16_t crc, uint8_t *buffer, uint32_t len)
{
    while (len--) {
        crc = crc16_byte(crc, *buffer++);
    }
    return crc;
}

void spi_slave_check_transfer_data(SPI_Type *ptr)
{
    uint16_t calc_crc, crc;
    /* Wait for the spi slave transfer to complete */
    while (spi_transfer_done == false) {
    }
    spi_transfer_done = false;
    calc_crc = crc16(0xFFFF, receive_buff_trans[rx_count], TEST_TRANSFER_DATA_IN_BYTE - 2);
    crc = *(uint16_t *)&receive_buff_trans[rx_count][TEST_TRANSFER_DATA_IN_BYTE - 2];
    if (calc_crc == crc) {
        printf("o 0x%04x %d\n", crc, rx_count);
    }
    rx_count++;
    if (rx_count == TEST_TRANSFER_DMA_TRANS_CONT) {
        rx_count = 0;
    }
}

int main(void)
{
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    hpm_stat_t stat;
    uint8_t cmd = 0x0;
    uint32_t addr = 0x0;
    uint32_t spi_tx_trans_count, spi_rx_trans_count;

    board_init();
    board_init_spi_clock(TEST_SPI);
    board_init_spi_pins(TEST_SPI);
    printf("SPI Slave DMA Transfer Example\n");
    intc_m_enable_irq_with_priority(BOARD_APP_HDMA_IRQ, 1);
    intc_m_enable_irq_with_priority(TEST_SPI_IRQ, 2);
    /* set SPI format config for slave */
    spi_slave_get_default_format_config(&format_config);
    format_config.common_config.data_len_in_bits = TEST_SPI_DATA_LEN_IN_BIT;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = false;
    format_config.common_config.mode = spi_slave_mode;
    format_config.common_config.cpol = spi_sclk_high_idle;
    format_config.common_config.cpha = spi_sclk_sampling_even_clk_edges;
    spi_format_init(TEST_SPI, &format_config);
    spi_enable_interrupt(TEST_SPI,  spi_end_int);
    spi_set_rx_fifo_threshold(TEST_SPI, 1U);
    
    // dma_enable_channel_interrupt(TEST_SPI_DMA, TEST_SPI_RX_DMA_CH, DMA_INTERRUPT_MASK_TERMINAL_COUNT);
    /* set SPI control config for slave */
    spi_slave_get_default_control_config(&control_config);
    control_config.slave_config.slave_data_only = true;
    control_config.common_config.tx_dma_enable = true;
    control_config.common_config.rx_dma_enable = true;
    control_config.common_config.trans_mode = spi_trans_write_read_together;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;

    spi_tx_trans_count = TEST_TRANSFER_DATA_IN_BYTE / TEST_SPI_DATA_LEN_IN_BYTE;
    spi_rx_trans_count = TEST_TRANSFER_DATA_IN_BYTE / TEST_SPI_DATA_LEN_IN_BYTE;
        /* setup spi tx trigger dma transfer */
    stat = spi_setup_dma_transfer(TEST_SPI,
                    &control_config,
                    &cmd, &addr,
                    spi_tx_trans_count, spi_rx_trans_count);
    if (stat != status_success) {
        printf("spi setup dma transfer failed\n");
        while (1) {
        }
    }
    stat = spi_rx_trigger_dma();
    while (1) {
        spi_slave_check_transfer_data(TEST_SPI);
    }

    return 0;
}
