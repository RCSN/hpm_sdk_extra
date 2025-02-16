 /*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_l1c_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_dma_mgr.h"

#define TEST_SPI               BOARD_APP_SPI_BASE
#define TEST_SPI_SCLK_FREQ     BOARD_APP_SPI_SCLK_FREQ
#define TEST_APP_SPI_RX_DMA     BOARD_APP_SPI_RX_DMA
#define TEST_SPI_DATA_LEN_IN_BIT          (8U)

#define SPI_MULTI_CS_COUNT         (2U)
#define SPI_TRANS_DATA_BUFF_SIZE   (10U)

static void spi_rx_dma_mgr_generate(SPI_Type *ptr, dma_resource_t *resource,
                                    dma_mgr_linked_descriptor_t *descriptor,
                                    uint8_t *buffer0, uint32_t buffer0_size,
                                    uint8_t *buffer1, uint32_t buffer1_size);

ATTR_PLACE_AT_NONCACHEABLE uint8_t receive_buff[SPI_MULTI_CS_COUNT][SPI_TRANS_DATA_BUFF_SIZE];
ATTR_PLACE_AT_NONCACHEABLE uint32_t spi_ctrl_reg[2];
ATTR_PLACE_AT_NONCACHEABLE uint32_t cmd_dummy_value;
ATTR_PLACE_AT_NONCACHEABLE uint32_t spi_rx_size;

/* rd_trans_cnt(transctrl) -> cmd -> spi_cs0_data -> cs0_switch_cs1 ->
* rd_trans_cnt(transctrl) -> cmd -> spi_cs1_data -> cs1_switch_cs0 */
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(8)
dma_mgr_linked_descriptor_t dma_linked_descriptor[8];

 dma_resource_t spi_rx_dma_resource;

int main(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    uint32_t spi_clock;
    board_init();
    dma_mgr_init();
    spi_clock = board_init_spi_clock(TEST_SPI);
    board_init_spi_pins(TEST_SPI);
    printf("SPI Master DMA Transfer Example\n");

    /* set SPI sclk frequency for master */
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = spi_clock;
    timing_config.master_config.sclk_freq_in_hz = TEST_SPI_SCLK_FREQ;
    if (status_success != spi_master_timing_init(TEST_SPI, &timing_config)) {
        printf("SPI master timming init failed\n");
        while (1) {
        }
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
    control_config.common_config.trans_mode = spi_trans_read_only;
    if (status_success != spi_control_init(TEST_SPI, &control_config, 0,
                                        SPI_TRANS_DATA_BUFF_SIZE)) {
        printf("spi control init failed\n");
        while (1) {
        }
    }
    spi_enable_rx_dma(TEST_SPI);

    if (dma_mgr_request_resource(&spi_rx_dma_resource) != status_success) {
        printf("spi rx dma request resource failed\n");
        while (1) {
        }
    }
    printf("spi rx dma channel is %d\n", spi_rx_dma_resource.channel);
    spi_rx_dma_mgr_generate(TEST_SPI, &spi_rx_dma_resource,
                            &dma_linked_descriptor[0], receive_buff[0],
                            SPI_TRANS_DATA_BUFF_SIZE, receive_buff[1],
                            SPI_TRANS_DATA_BUFF_SIZE);
    dma_mgr_enable_channel(&spi_rx_dma_resource);
    while (1) {
    }
 }

static void spi_rx_dma_mgr_generate(SPI_Type *ptr, dma_resource_t *resource, dma_mgr_linked_descriptor_t* descriptor,
                                    uint8_t *buffer0, uint32_t buffer0_size, uint8_t *buffer1, uint32_t buffer1_size)
{
    dma_mgr_chn_conf_t chg_config;
    spi_ctrl_reg[0] = (ptr->CTRL & ~SPI_CTRL_CS_EN_MASK) | SPI_CTRL_CS_EN_SET(spi_cs_0);
    spi_ctrl_reg[1] = (ptr->CTRL & ~SPI_CTRL_CS_EN_MASK) | SPI_CTRL_CS_EN_SET(spi_cs_1);
    cmd_dummy_value = 0xFF;

    dma_mgr_get_default_chn_config(&chg_config);

#if defined(HPM_IP_FEATURE_SPI_NEW_TRANS_COUNT) && (HPM_IP_FEATURE_SPI_NEW_TRANS_COUNT == 1)
    /*SPI RD_TRANS_CNT cs0*/
    spi_rx_size = buffer0_size - 1;
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_rx_size);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->RD_TRANS_CNT);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[1];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[0]);
#else
    /*SPI TRANSCTRL cs0*/
    spi_rx_size = (ptr->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(buffer0_size - 1);
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_rx_size);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->TRANSCTRL);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[1];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[0]);
#endif
    /* SPI CMD*/
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&cmd_dummy_value);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->CMD);
    chg_config.linked_ptr = (uint32_t)&descriptor[2];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[1]);

    /* SPI CS0 DATA */
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
    chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
    chg_config.size_in_byte = buffer0_size;
    chg_config.src_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->DATA);
    chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buffer0);
    chg_config.linked_ptr = (uint32_t)&descriptor[3];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[2]);

    /*SPI CTRL cs0 -> cs1 */
    /* Only MCUs after HPM5300 are supported */
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_ctrl_reg[1]);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->CTRL);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[4];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[3]);

#if defined(HPM_IP_FEATURE_SPI_NEW_TRANS_COUNT) && (HPM_IP_FEATURE_SPI_NEW_TRANS_COUNT == 1)
    /*SPI RD_TRANS_CNT cs1*/
    spi_rx_size = buffer1_size - 1;
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_rx_size);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->RD_TRANS_CNT);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[5];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[4]);
#else
    /*SPI TRANSCTRL cs0*/
    spi_rx_size = (ptr->TRANSCTRL & ~SPI_TRANSCTRL_RDTRANCNT_MASK) | SPI_TRANSCTRL_RDTRANCNT_SET(buffer1_size - 1);
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_rx_size);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->TRANSCTRL);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[5];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[4]);
#endif

    /* SPI CMD*/
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.size_in_byte = sizeof(uint8_t);
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&cmd_dummy_value);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->CMD);
    chg_config.linked_ptr = (uint32_t)&descriptor[6];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[5]);

    /* SPI CS1 DATA */
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
    chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
    chg_config.size_in_byte = buffer1_size;
    chg_config.src_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->DATA);
    chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buffer1);
    chg_config.linked_ptr = (uint32_t)&descriptor[7];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[6]);

    /* SPI CTRL cs1 -> cs0 */
    /* Only MCUs after HPM5300 are supported */
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_WORD;
    chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    chg_config.src_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&spi_ctrl_reg[0]);
    chg_config.dst_addr = (uint32_t)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)&ptr->CTRL);
    chg_config.size_in_byte = sizeof(uint32_t);
    chg_config.linked_ptr = (uint32_t)&descriptor[0];
    dma_mgr_config_linked_descriptor(resource, &chg_config, &descriptor[7]);

    chg_config.en_dmamux = true;
    chg_config.dmamux_src = TEST_APP_SPI_RX_DMA;
    chg_config.linked_ptr = (uint32_t)&descriptor[0];
    dma_mgr_setup_channel(resource, &chg_config);
}


