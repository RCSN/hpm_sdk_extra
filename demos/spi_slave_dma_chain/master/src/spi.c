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
#define TEST_SPI_SCLK_FREQ     BOARD_APP_SPI_SCLK_FREQ
#define TEST_SPI_DMA           BOARD_APP_HDMA
#define TEST_SPI_DMAMUX        BOARD_APP_DMAMUX
#define TEST_SPI_RX_DMA_REQ    BOARD_APP_SPI_RX_DMA
#define TEST_SPI_TX_DMA_REQ    BOARD_APP_SPI_TX_DMA
#define TEST_SPI_RX_DMA_CH     0
#define TEST_SPI_TX_DMA_CH     1
#define TEST_SPI_RX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(TEST_SPI_DMA, TEST_SPI_RX_DMA_CH)
#define TEST_SPI_TX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(TEST_SPI_DMA, TEST_SPI_TX_DMA_CH)

/* data width definition */
#define TEST_SPI_DATA_LEN_IN_BIT          (8U)
#define TEST_SPI_DATA_LEN_IN_BYTE         (1U)
#define TEST_SPI_DMA_TRANS_DATA_WIDTH     DMA_TRANSFER_WIDTH_BYTE

#ifndef PLACE_BUFF_AT_CACHEABLE
#define PLACE_BUFF_AT_CACHEABLE 1
#endif

#define TEST_TRANSFER_DATA_IN_BYTE  (260U)
#if PLACE_BUFF_AT_CACHEABLE
ATTR_ALIGN(HPM_L1C_CACHELINE_SIZE) uint8_t sent_buff[TEST_TRANSFER_DATA_IN_BYTE];
ATTR_ALIGN(HPM_L1C_CACHELINE_SIZE) uint8_t receive_buff[TEST_TRANSFER_DATA_IN_BYTE];
#else
ATTR_PLACE_AT_NONCACHEABLE uint8_t sent_buff[TEST_TRANSFER_DATA_IN_BYTE];
ATTR_PLACE_AT_NONCACHEABLE uint8_t receive_buff[TEST_TRANSFER_DATA_IN_BYTE];
#endif

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

void prepare_transfer_data(void)
{
    *(uint32_t *)sent_buff = rand();
    for (uint32_t i = 4; i < TEST_TRANSFER_DATA_IN_BYTE - 2; i++) {
        sent_buff[i] = i % 0xFF;
    }
    *(uint16_t *)&sent_buff[TEST_TRANSFER_DATA_IN_BYTE - 2] = crc16(0xffff, sent_buff, TEST_TRANSFER_DATA_IN_BYTE - 2);
}

void spi_master_check_transfer_data(SPI_Type *ptr)
{
    uint32_t i = 0U;

    /* Wait for the spi master transfer to complete */
    while (spi_is_active(ptr)) {
    }
    /* disable spi dma before starting next dma transaction */
    spi_disable_tx_dma(ptr);
    spi_disable_rx_dma(ptr);

    printf("crc:0x%04x\n", *(uint16_t *)&sent_buff[TEST_TRANSFER_DATA_IN_BYTE - 2]);
    //for (i = TEST_TRANSFER_DATA_IN_BYTE - 2; i < TEST_TRANSFER_DATA_IN_BYTE; i++) {
    //    if ((i & 0x0FU) == 0U) {
    //        printf("\r\n");
    //    }
    //    printf("0x%02X ", sent_buff[i]);
    //}
    //printf("\n");
}

hpm_stat_t spi_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr, uint32_t src, uint8_t data_width, uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = (uint32_t)&spi_ptr->DATA;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = data_width;
    config.size_in_byte = size;

    return dma_setup_handshake(dma_ptr, &config, true);
}

hpm_stat_t spi_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr, uint32_t dst, uint8_t data_width, uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = dst;
    config.dst_fixed = true;
    config.src = (uint32_t)&spi_ptr->DATA;
    config.src_fixed = true;
    config.data_width = data_width;
    config.size_in_byte = size;

    return dma_setup_handshake(dma_ptr, &config, true);
}

int main(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    hpm_stat_t stat;
    uint32_t spi_clcok;
    uint8_t cmd = 0x1a;
    uint32_t addr = 0x10;
    uint32_t spi_tx_trans_count, spi_rx_trans_count;

    board_init();
    spi_clcok = board_init_spi_clock(TEST_SPI);
    board_init_spi_pins(TEST_SPI);
    printf("SPI Master DMA Transfer Example\n");

    /* set SPI sclk frequency for master */
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = spi_clcok;
    timing_config.master_config.sclk_freq_in_hz = TEST_SPI_SCLK_FREQ;
    if (status_success != spi_master_timing_init(TEST_SPI, &timing_config)) {
        printf("SPI master timing init failed\n");
    }

    /* set SPI format config for master */
    spi_master_get_default_format_config(&format_config);
    format_config.master_config.addr_len_in_bytes = 1U;
    format_config.common_config.data_len_in_bits = TEST_SPI_DATA_LEN_IN_BIT;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = false;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_high_idle;
    format_config.common_config.cpha = spi_sclk_sampling_even_clk_edges;
    spi_format_init(TEST_SPI, &format_config);

    /* set SPI control config for master */
    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
    control_config.common_config.tx_dma_enable = true;
    control_config.common_config.rx_dma_enable = true;
    control_config.common_config.trans_mode = spi_trans_write_read_together;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;

    spi_tx_trans_count = sizeof(sent_buff) / TEST_SPI_DATA_LEN_IN_BYTE;
    spi_rx_trans_count = sizeof(receive_buff) / TEST_SPI_DATA_LEN_IN_BYTE;

    while (1) {
    prepare_transfer_data();
    stat = spi_setup_dma_transfer(TEST_SPI,
                        &control_config,
                        &cmd, &addr,
                        spi_tx_trans_count, spi_rx_trans_count);
    if (stat != status_success) {
        printf("spi setup dma transfer failed\n");
        while (1) {
        }
    }
    /* setup spi tx trigger dma transfer*/
#if PLACE_BUFF_AT_CACHEABLE
    if (l1c_dc_is_enabled()) {
        /* cache writeback for sent buff */
        uint32_t aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)sent_buff);
        uint32_t aligned_end = HPM_L1C_CACHELINE_ALIGN_UP((uint32_t)sent_buff + sizeof(sent_buff));
        uint32_t aligned_size = aligned_end - aligned_start;
        l1c_dc_writeback(aligned_start, aligned_size);
    }
#endif
    dmamux_config(TEST_SPI_DMAMUX, TEST_SPI_TX_DMAMUX_CH, TEST_SPI_TX_DMA_REQ, true);
    stat = spi_tx_trigger_dma(TEST_SPI_DMA,
                            TEST_SPI_TX_DMA_CH,
                            TEST_SPI,
                            core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)sent_buff),
                            TEST_SPI_DMA_TRANS_DATA_WIDTH,
                            sizeof(sent_buff));
    if (stat != status_success) {
        printf("spi tx trigger dma failed\n");
        while (1) {
        }
    }
    uint8_t dummy = 0;
    /* setup spi rx trigger dma transfer*/
    dmamux_config(TEST_SPI_DMAMUX, TEST_SPI_RX_DMAMUX_CH, TEST_SPI_RX_DMA_REQ, true);
    stat = spi_rx_trigger_dma(TEST_SPI_DMA,
                            TEST_SPI_RX_DMA_CH,
                            TEST_SPI,
                            core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&dummy),
                            TEST_SPI_DMA_TRANS_DATA_WIDTH,
                            TEST_TRANSFER_DATA_IN_BYTE);
    if (stat != status_success) {
        printf("spi rx trigger dma failed\n");
        while (1) {
        }
    }
    spi_master_check_transfer_data(TEST_SPI);
    board_delay_ms(2);
    }

    return 0;
}
