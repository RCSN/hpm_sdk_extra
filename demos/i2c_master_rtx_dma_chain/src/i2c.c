/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_i2c_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_i2c_dma_chain.h"

#define TEST_I2C               BOARD_APP_I2C_BASE
#define TEST_I2C_CLOCK_NAME    BOARD_APP_I2C_CLK_NAME
#define TEST_I2C_DMA           BOARD_APP_I2C_DMA
#define TEST_I2C_DMAMUX        BOARD_APP_I2C_DMAMUX
#define TEST_I2C_DMAMUX_SRC    BOARD_APP_I2C_DMA_SRC
#define TEST_I2C_DMA_CH        0
#define TEST_I2C_DMAMUX_CH     DMA_SOC_CHN_TO_DMAMUX_CHN(TEST_I2C_DMA, TEST_I2C_DMA_CH)

#define TEST_I2C_SLAVE_ADDRESS (0x16U)

#define TEST_TRANSFER_DATA_IN_BYTE  (128U)
ATTR_PLACE_AT_NONCACHEABLE uint8_t rx_buff[TEST_TRANSFER_DATA_IN_BYTE];
ATTR_PLACE_AT_NONCACHEABLE uint8_t tx_buff[TEST_TRANSFER_DATA_IN_BYTE];

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t tx_descriptors[5];
ATTR_PLACE_AT_NONCACHEABLE hpm_i2c_cfg_t tx_i2c_cfg;
ATTR_PLACE_AT_NONCACHEABLE hpm_i2c_dma_chain_cfg_t tx_i2c_dma_cfg;

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dma_linked_descriptor_t rx_descriptors[5];
ATTR_PLACE_AT_NONCACHEABLE hpm_i2c_cfg_t rx_i2c_cfg;
ATTR_PLACE_AT_NONCACHEABLE hpm_i2c_dma_chain_cfg_t rx_i2c_dma_cfg;

static void i2c_handle_dma_transfer_complete(I2C_Type *ptr)
{
    volatile uint32_t status;

    /* wait for i2c transaction complete */
    do {
        status = i2c_get_status(ptr);
    } while (!(status & I2C_STATUS_CMPL_MASK));
    /* clear status */
    i2c_clear_status(ptr, status);
    /* disable i2c dma before next dma transaction */
    i2c_dma_disable(ptr);
}

static void prepare_tx_data(void)
{
    for (uint8_t i = 0; i < TEST_TRANSFER_DATA_IN_BYTE; i++) {
        tx_buff[i] = i % 0xFF;
    }
}

static void check_transfer_data(void)
{
    uint32_t i = 0U, error_count = 0U;

    printf("The sent data are:");
    for (i = 0; i < TEST_TRANSFER_DATA_IN_BYTE; i++) {
        if ((i & 0x0FU) == 0U) {
            printf("\r\n");
        }
        printf("0x%02X ", tx_buff[i]);
    }
    printf("\n");
    printf("The received data are:");
    for (i = 0; i < TEST_TRANSFER_DATA_IN_BYTE; i++) {
        if ((i & 0x0FU) == 0U) {
            printf("\n");
        }
        printf("0x%02X ", rx_buff[i]);
        if (tx_buff[i] != rx_buff[i]) {
            error_count++;
        }
    }
    printf("\n");
    if (error_count == 0) {
        printf("I2C transfer all data matched!\n");
    } else {
        printf("Error occurred in I2C transfer!\n");
    }
}

static void i2c_dma_chain_cfg_init(void)
{
    tx_i2c_cfg.dev_addr = TEST_I2C_SLAVE_ADDRESS;
    tx_i2c_dma_cfg.descriptors = tx_descriptors;
    tx_i2c_dma_cfg.dma_ch = TEST_I2C_DMA_CH;
    tx_i2c_dma_cfg.dma_ptr = TEST_I2C_DMA;
    tx_i2c_dma_cfg.i2c_ptr = TEST_I2C;

    rx_i2c_cfg.dev_addr = TEST_I2C_SLAVE_ADDRESS;
    rx_i2c_dma_cfg.descriptors = rx_descriptors;
    rx_i2c_dma_cfg.dma_ch = TEST_I2C_DMA_CH;
    rx_i2c_dma_cfg.dma_ptr = TEST_I2C_DMA;
    rx_i2c_dma_cfg.i2c_ptr = TEST_I2C;
}

int main(void)
{
    hpm_stat_t stat;
    i2c_config_t config;
    uint32_t freq;
    bool read = false;
    bool write = false;

    board_init();
    init_i2c_pins(TEST_I2C);

    config.i2c_mode = i2c_mode_normal;
    config.is_10bit_addressing = false;
    freq = clock_get_frequency(TEST_I2C_CLOCK_NAME);
    stat = i2c_init_master(TEST_I2C, freq, &config);
    if (stat != status_success) {
        return stat;
    }

    printf("I2C DMA chain master example\n");
    prepare_tx_data();

    dmamux_config(TEST_I2C_DMAMUX, TEST_I2C_DMAMUX_CH, TEST_I2C_DMAMUX_SRC, true);
    
    i2c_dma_chain_cfg_init();
    hpm_i2c_master_transfer_dma_chain_generate(&tx_i2c_dma_cfg, tx_buff, TEST_TRANSFER_DATA_IN_BYTE, &tx_i2c_cfg, false);
    hpm_i2c_master_transfer_dma_chain_generate(&rx_i2c_dma_cfg, rx_buff, TEST_TRANSFER_DATA_IN_BYTE, &rx_i2c_cfg, true);

    hpm_i2c_master_transfer_dma_chain_setup(&tx_i2c_dma_cfg, &tx_i2c_cfg, false);
    hpm_i2c_master_transfer_dma_chain_setup(&rx_i2c_dma_cfg, &rx_i2c_cfg, true);

    while (1) {
        if (write == false) {
            write = true;
            hpm_i2c_master_transfer_dma_chain_start(&tx_i2c_dma_cfg, &tx_i2c_cfg, false);
        }
        if (i2c_get_status(tx_i2c_dma_cfg.i2c_ptr) & I2C_STATUS_CMPL_MASK) {
            if ((write == true) && (read == false)) {
                read = true;
                hpm_i2c_master_transfer_dma_chain_start(&rx_i2c_dma_cfg, &rx_i2c_cfg, true);
            } else if ((write == true) && (read == true)) {
                check_transfer_data(); /* printf data */
                write = false;
                read = false;
            }
        }
    }
    return 0;
}

