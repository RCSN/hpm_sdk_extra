/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifdef CONFIG_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_common.h"
#include "hpm_l1c_drv.h"
#include "hpm_spi_drv.h"
#include "board.h"
#include "wizchip_conf.h"

#ifndef USE_HARDWARE_CS
#define USE_HARDWARE_CS  0
#endif

#define PORT_CS_PIN            BOARD_SPI_CS_PIN
#define PORT_SPI_BASE          BOARD_APP_SPI_BASE
#define PORT_SPI_SCLK_FREQ     BOARD_APP_SPI_SCLK_FREQ
#define PORT_SPI_DMA           BOARD_APP_HDMA
#define PORT_SPI_DMAMUX        BOARD_APP_DMAMUX
#define PORT_SPI_RX_DMA_REQ    BOARD_APP_SPI_RX_DMA
#define PORT_SPI_TX_DMA_REQ    BOARD_APP_SPI_TX_DMA
#define PORT_SPI_RX_DMA_CH     0
#define PORT_SPI_TX_DMA_CH     1
#define PORT_SPI_RX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(PORT_SPI_DMA, PORT_SPI_RX_DMA_CH)
#define PORT_SPI_TX_DMAMUX_CH  DMA_SOC_CHN_TO_DMAMUX_CHN(PORT_SPI_DMA, PORT_SPI_TX_DMA_CH)

static hpm_stat_t spi_nor_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num,
                                         SPI_Type *spi_ptr, uint32_t src,
                                         uint8_t data_width, uint32_t size,
                                         uint8_t burst_size);
static hpm_stat_t spi_nor_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num,
                                         SPI_Type *spi_ptr, uint32_t dst,
                                         uint8_t data_width, uint32_t size,
                                         uint8_t burst_size);
static hpm_stat_t hpm_spi_transfer_via_dma(SPI_Type *spi_ptr, DMA_Type *dma_ptr,
                                           uint8_t *buf, uint32_t len,
                                           bool is_read);
static void cris_en(void);
static void cris_ex(void);
static void cs_sel(void);
static void actual_cs_sel(void);
static void cs_desel(void);
static uint8_t spi_rbyte(void);
static void spi_wbyte(uint8_t wb);
static void spi_rbusrt(uint8_t *pBuf, uint16_t len);
static void spi_wburst(uint8_t *pBuf, uint16_t len);

static hpm_stat_t spi_nor_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num,
                                         SPI_Type *spi_ptr, uint32_t src,
                                         uint8_t data_width, uint32_t size,
                                         uint8_t burst_size)
{
    hpm_stat_t stat;
    dma_channel_config_t config;
    if (ch_num >= DMA_SOC_CHANNEL_NUM) {
        return status_invalid_argument;
    }
    dma_default_channel_config(dma_ptr, &config);
    config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    config.dst_mode      = DMA_HANDSHAKE_MODE_HANDSHAKE;
    config.src_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
    config.src_mode      = DMA_HANDSHAKE_MODE_NORMAL;
    config.src_width     = data_width;
    config.dst_width     = data_width;
    config.src_addr      = src;
    config.dst_addr      = (uint32_t)&spi_ptr->DATA;
    config.size_in_byte  = size;
    config.src_burst_size = burst_size;
    stat = dma_setup_channel(dma_ptr, ch_num, &config, true);
    if (stat != status_success) {
        return stat;
    }
    return stat;
}

static hpm_stat_t spi_nor_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr, uint32_t dst,
                                            uint8_t data_width, uint32_t size, uint8_t burst_size)
{
    hpm_stat_t stat;
    dma_channel_config_t config;
    if (ch_num >= DMA_SOC_CHANNEL_NUM) {
        return status_invalid_argument;
    }
    dma_default_channel_config(dma_ptr, &config);
    config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
    config.dst_mode      = DMA_HANDSHAKE_MODE_HANDSHAKE;
    config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    config.src_mode      = DMA_HANDSHAKE_MODE_NORMAL;
    config.src_width     = data_width;
    config.dst_width     = data_width;
    config.src_addr      = (uint32_t)&spi_ptr->DATA;
    config.dst_addr      = dst;
    config.size_in_byte  = size;
    config.src_burst_size = burst_size;
    stat = dma_setup_channel(dma_ptr, ch_num, &config, true);
    if (stat != status_success) {
        return stat;
    }
    return stat;
}

static hpm_stat_t hpm_spi_transfer_via_dma(SPI_Type *spi_ptr, DMA_Type *dma_ptr, uint8_t *buf, uint32_t len, bool is_read)
{
    hpm_stat_t stat;
    uint32_t timeout_count = 0;

    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(len - 1)) |
                                ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_WRTRANCNT_MASK) | SPI_TRANSCTRL_WRTRANCNT_SET(len - 1));
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
    PORT_SPI_BASE->WR_TRANS_CNT = len - 1;
    PORT_SPI_BASE->RD_TRANS_CNT = len - 1;
#endif
    if (is_read) {
        /* set mode is readonly. and set the transfer len is 1*/
        PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_read_only));
        /* reset the fifo*/
        PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
        stat = spi_nor_rx_trigger_dma(dma_ptr, PORT_SPI_RX_DMA_CH, spi_ptr,
                                core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buf),
                                DMA_TRANSFER_WIDTH_BYTE, len, DMA_NUM_TRANSFER_PER_BURST_1T);
        actual_cs_sel();
        PORT_SPI_BASE->CMD = 0xff;

        while (spi_is_active(spi_ptr)) {
            timeout_count++;
            if (timeout_count >= 0xFFFFFF) {
                stat = status_timeout;
                break;
            }
        }
    } else {
        /* set mode is readonly. and set the transfer len is 1*/
        PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_only));
        /* reset the fifo*/
        PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
        stat = spi_nor_tx_trigger_dma(dma_ptr, PORT_SPI_TX_DMA_CH, spi_ptr,
                                        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buf),
                                        DMA_TRANSFER_WIDTH_BYTE, len, DMA_NUM_TRANSFER_PER_BURST_1T);
        actual_cs_sel();
        PORT_SPI_BASE->CMD = 0xff;
        while (spi_is_active(spi_ptr)) {
            timeout_count++;
            if (timeout_count >= 0xFFFFFF) {
                stat = status_timeout;
                break;
            }
        }
    }
    return stat;
}


static void cris_en(void)
{
    // disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

static void cris_ex(void)
{
    // enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

static void cs_sel(void)
{
    /*in order to cs interval, actual_cs_sel API is actually use during transmisson*/

}

static void actual_cs_sel(void)
{
#if !defined(USE_HARDWARE_CS) || (USE_HARDWARE_CS == 0)
    board_write_spi_cs(PORT_CS_PIN, BOARD_SPI_CS_ACTIVE_LEVEL);
#endif
}

static void cs_desel(void)
{
#if !defined(USE_HARDWARE_CS) || (USE_HARDWARE_CS == 0)
    board_write_spi_cs(PORT_CS_PIN, !BOARD_SPI_CS_ACTIVE_LEVEL);
#endif
}

static uint8_t spi_rbyte(void)
{
    /* set mode is readonly. and set the transfer len is 1*/
    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_read_only));
    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(0));
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
    PORT_SPI_BASE->RD_TRANS_CNT = 0;
#endif
    /* reset the fifo*/
    PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
    /* start tranfer */
    actual_cs_sel();
    PORT_SPI_BASE->CMD = 0xff;
    /* read fifo one byte*/
    while (spi_is_active(PORT_SPI_BASE));
    return PORT_SPI_BASE->DATA;
}

static void spi_wbyte(uint8_t wb)
{
    /* set mode is readonly. and set the transfer len is 1*/
    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_only));
    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_WRTRANCNT_MASK) | SPI_TRANSCTRL_WRTRANCNT_SET(0));
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
    PORT_SPI_BASE->WR_TRANS_CNT = 0;
#endif
    /* reset the fifo*/
    PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
    /* write one byte for fifo*/
    
    /* start tranfer */
    actual_cs_sel();
    PORT_SPI_BASE->CMD = 0xff;
    PORT_SPI_BASE->DATA = wb;
    while (spi_is_active(PORT_SPI_BASE));
}

static void spi_rbusrt(uint8_t* pBuf, uint16_t len)
{
    hpm_stat_t stat = status_success;
    uint32_t i = 0;
    uint32_t aligned_start;
    uint32_t aligned_end;
    uint32_t aligned_size;
    uint16_t remaining_len = len;
    uint16_t read_size = 0;
    uint32_t temp;
    uint8_t *dst_8 = (uint8_t *) pBuf;
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
    PORT_SPI_BASE->WR_TRANS_CNT = len - 1;
    PORT_SPI_BASE->RD_TRANS_CNT = len - 1;
#endif
    if (len <= SPI_SOC_FIFO_DEPTH) {
        /* set mode is readonly. and set the transfer len is 1*/
        PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_read_only));
        PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(len - 1)) |
                                    ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_WRTRANCNT_MASK) | SPI_TRANSCTRL_WRTRANCNT_SET(len - 1));
        /* reset the fifo*/
        PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
        /* start tranfer */
        actual_cs_sel();
        PORT_SPI_BASE->CMD = 0xff;
        while (spi_is_active(PORT_SPI_BASE));
        for (i = 0; i < len;) {
            if(spi_get_rx_fifo_valid_data_size(PORT_SPI_BASE) != 0) {
                temp = PORT_SPI_BASE->DATA;
                pBuf[i] = temp;
                i++;
            }
        }
    } else {
        while(remaining_len > 0) {
            read_size = MIN(remaining_len, SPI_SOC_TRANSFER_COUNT_MAX);
            hpm_spi_transfer_via_dma(PORT_SPI_BASE, PORT_SPI_DMA, dst_8,
                                     read_size, true);
            HPM_BREAK_IF(stat != status_success);
            if (l1c_dc_is_enabled()) {
                /* cache invalidate for receive buff */
                aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dst_8);
                aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(dst_8 + read_size);
                aligned_size = aligned_end - aligned_start;
                l1c_dc_invalidate(aligned_start, aligned_size);
            }
            remaining_len -= read_size;
            dst_8 += read_size;
        }
    }
}

static void spi_wburst(uint8_t* pBuf, uint16_t len)
{
    hpm_stat_t stat = status_success;
    uint32_t i = 0;
    uint32_t aligned_start;
    uint32_t aligned_end;
    uint32_t aligned_size;
    uint16_t remaining_len = len;
    uint16_t read_size = 0;
    uint8_t *dst_8 = (uint8_t *) pBuf;
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
        PORT_SPI_BASE->WR_TRANS_CNT = len - 1;
        PORT_SPI_BASE->RD_TRANS_CNT = len - 1;
#endif
    if (len <= SPI_SOC_FIFO_DEPTH) {
        /* set mode is readonly. and set the transfer len is 1*/
        PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_only));
        PORT_SPI_BASE->TRANSCTRL =  ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(0)) |
                                ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_WRTRANCNT_MASK) | SPI_TRANSCTRL_WRTRANCNT_SET(0));
        /* reset the fifo*/
        PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
        actual_cs_sel();
        PORT_SPI_BASE->CMD = 0xff;
        for (i = 0; i < len; i++) {
            PORT_SPI_BASE->DATA = pBuf[i];
        }
        /* start tranfer */
        while (spi_is_active(PORT_SPI_BASE));
    } else {
        while(remaining_len > 0) {
            read_size = MIN(remaining_len, SPI_SOC_TRANSFER_COUNT_MAX);
            if (l1c_dc_is_enabled()) {
                /* cache writeback for sent buff */
                aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dst_8);
                aligned_end = HPM_L1C_CACHELINE_ALIGN_UP((uint32_t)dst_8 + read_size);
                aligned_size = aligned_end - aligned_start;
                l1c_dc_writeback(aligned_start, aligned_size);
            }
            hpm_spi_transfer_via_dma(PORT_SPI_BASE, PORT_SPI_DMA, dst_8,
                                     read_size, false);
            HPM_BREAK_IF(stat != status_success);
            remaining_len -= read_size;
            dst_8 += read_size;
        }
    }
}

uint8_t wizchip_read_byte(uint8_t *addr_sel, uint8_t addr_sel_len)
{
    uint8_t i = 0;
    /* set mode is readonly. and set the transfer len is 1*/
    PORT_SPI_BASE->TRANSCTRL = ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_TRANSMODE_MASK) | SPI_TRANSCTRL_TRANSMODE_SET(spi_trans_write_read));
    PORT_SPI_BASE->TRANSCTRL =  ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(0)) |
                                ((PORT_SPI_BASE->TRANSCTRL & ~SPI_TRANSCTRL_WRTRANCNT_MASK) | SPI_TRANSCTRL_WRTRANCNT_SET(addr_sel_len - 1));
#if defined(SPI_SOC_HAS_NEW_TRANS_COUNT) && (SPI_SOC_HAS_NEW_TRANS_COUNT == 1)
    PORT_SPI_BASE->WR_TRANS_CNT = addr_sel_len - 1;
    PORT_SPI_BASE->RD_TRANS_CNT = 0;
#endif
    /* reset the fifo*/
    PORT_SPI_BASE->CTRL |= SPI_CTRL_TXFIFORST_MASK | SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_SPIRST_MASK;
    /* write one byte for fifo*/
    
    /* start tranfer */
    actual_cs_sel();
    PORT_SPI_BASE->CMD = 0xff;
    for (i = 0; i < addr_sel_len; i++) {
        PORT_SPI_BASE->DATA = addr_sel[i];
    }
    while (spi_is_active(PORT_SPI_BASE));
    cs_desel();
    return PORT_SPI_BASE->DATA;
}

void wizchip_register_port(void)
{
    reg_wizchip_cris_cbfunc(cris_en, cris_ex);            /*critical section*/
    reg_wizchip_cs_cbfunc(cs_sel, cs_desel);              /* cs register */
    reg_wizchip_spi_cbfunc(spi_rbyte, spi_wbyte);         /* byte taransfer register*/
    reg_wizchip_spiburst_cbfunc(spi_rbusrt, spi_wburst);  /* block transfer register*/
}

void wizchip_spi_init(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    uint32_t spi_clcok;
#if !defined(USE_HARDWARE_CS) || (USE_HARDWARE_CS == 0)
    board_init_spi_pins_with_gpio_as_cs(PORT_SPI_BASE);
#else
     board_init_spi_pins(PORT_SPI_BASE);
#endif
    /* set SPI sclk frequency for master */
    spi_clcok = board_init_spi_clock(PORT_SPI_BASE);
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_1;
    timing_config.master_config.csht = spi_csht_half_sclk_1;
    timing_config.master_config.clk_src_freq_in_hz = spi_clcok;
    timing_config.master_config.sclk_freq_in_hz = PORT_SPI_SCLK_FREQ;
    if (status_success != spi_master_timing_init(PORT_SPI_BASE, &timing_config)) {
        printf("SPI master timing init failed\n");
    }
    /* set SPI format config for master */
    spi_master_get_default_format_config(&format_config);
    format_config.master_config.addr_len_in_bytes = 1U;
    format_config.common_config.data_len_in_bits = 8;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = false;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_high_idle;
    format_config.common_config.cpha = spi_sclk_sampling_even_clk_edges;
    spi_format_init(PORT_SPI_BASE, &format_config);

    spi_master_get_default_control_config(&control_config);
    control_config.common_config.tx_dma_enable = true;
    control_config.common_config.rx_dma_enable = true;
    control_config.common_config.trans_mode = spi_trans_write_read;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;
    spi_control_init(PORT_SPI_BASE, &control_config, 3, 1);
    PORT_SPI_BASE->CTRL |= SPI_CTRL_TXDMAEN_MASK | SPI_CTRL_RXDMAEN_MASK;
    dmamux_config(PORT_SPI_DMAMUX, PORT_SPI_TX_DMAMUX_CH, PORT_SPI_TX_DMA_REQ, true);
    dmamux_config(PORT_SPI_DMAMUX, PORT_SPI_RX_DMAMUX_CH, PORT_SPI_RX_DMA_REQ, true);
}
