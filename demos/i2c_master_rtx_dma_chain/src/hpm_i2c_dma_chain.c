/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "hpm_i2c_dma_chain.h"

hpm_stat_t hpm_i2c_master_transfer_dma_chain_start(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, hpm_i2c_cfg_t *i2c_cfg, bool read)
{
    (void)read;
    (void)i2c_cfg;
    /* step 1: clear CMPL bit to avoid blocking the transmission */
    i2c_clear_status(i2c_dma_cfg->i2c_ptr, I2C_STATUS_CMPL_MASK);
    i2c_dma_cfg->dma_ptr->CHCTRL[i2c_dma_cfg->dma_ch].LLPOINTER = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[0]);
    dma_set_transfer_size(i2c_dma_cfg->dma_ptr, i2c_dma_cfg->dma_ch, 1);
    dma_enable_channel(i2c_dma_cfg->dma_ptr, i2c_dma_cfg->dma_ch);
    return status_success;
}

hpm_stat_t hpm_i2c_master_transfer_dma_chain_setup(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, hpm_i2c_cfg_t *i2c_cfg, bool read)
{
    hpm_stat_t stat;
    dma_channel_config_t ch_config = { 0 };
    (void)read;
    dma_default_channel_config(i2c_dma_cfg->dma_ptr, &ch_config);
    /* step 1: clear CMPL bit to avoid blocking the transmission */
    i2c_clear_status(i2c_dma_cfg->i2c_ptr,I2C_STATUS_CMPL_MASK);
    /* step2: dma transfer i2c ADDR reg */
    ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->ADDR;
    ch_config.dst_width = DMA_TRANSFER_WIDTH_BYTE;
    ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_cfg->dev_addr);
    ch_config.src_width = DMA_TRANSFER_WIDTH_BYTE;
    ch_config.size_in_byte = 1;
    ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    ch_config.linked_ptr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[0]);
    stat = dma_setup_channel(i2c_dma_cfg->dma_ptr, i2c_dma_cfg->dma_ch, &ch_config, false);
    if (stat != status_success) {
        while (1) {
        };
    }
    return status_success;
}
hpm_stat_t hpm_i2c_master_transfer_dma_chain_generate(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, uint8_t *data, uint16_t data_size, hpm_i2c_cfg_t *i2c_cfg, bool read)
{
    dma_channel_config_t ch_config = { 0 };
    hpm_stat_t stat;
    dma_default_channel_config(i2c_dma_cfg->dma_ptr, &ch_config);
    ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;

    /* step 1: set start phase. data phase, stop phase */
    i2c_cfg->ctrl_val = I2C_CTRL_PHASE_START_MASK | I2C_CTRL_PHASE_STOP_MASK | I2C_CTRL_PHASE_ADDR_MASK
          | I2C_CTRL_PHASE_DATA_MASK
#ifdef I2C_CTRL_DATACNT_HIGH_MASK
          | I2C_CTRL_DATACNT_HIGH_SET(I2C_DATACNT_MAP(data_size) >> 8U)
#endif
          | I2C_CTRL_DATACNT_SET(I2C_DATACNT_MAP(data_size));
    if (read == true) {
        i2c_cfg->ctrl_val |= I2C_CTRL_DIR_SET(I2C_DIR_MASTER_READ);
    } else {
        i2c_cfg->ctrl_val |= I2C_CTRL_DIR_SET(I2C_DIR_MASTER_WRITE);
    }
    ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->CTRL;
    ch_config.dst_width = DMA_TRANSFER_WIDTH_WORD;
    ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_cfg->ctrl_val);
    ch_config.src_width = DMA_TRANSFER_WIDTH_WORD;
    ch_config.size_in_byte = 4;
    ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    ch_config.linked_ptr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[1]);
    HPM_CHECK_RET(dma_config_linked_descriptor(i2c_dma_cfg->dma_ptr, &i2c_dma_cfg->descriptors[0], i2c_dma_cfg->dma_ch, &ch_config));

    /* step 2: set dma */
    i2c_cfg->setup_val = i2c_dma_cfg->i2c_ptr->SETUP | I2C_SETUP_DMAEN_MASK;
    ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->SETUP;
    ch_config.dst_width = DMA_TRANSFER_WIDTH_WORD;
    ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_cfg->setup_val);
    ch_config.src_width = DMA_TRANSFER_WIDTH_WORD;
    ch_config.size_in_byte = 4;
    ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    ch_config.linked_ptr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[2]);
    HPM_CHECK_RET(dma_config_linked_descriptor(i2c_dma_cfg->dma_ptr, &i2c_dma_cfg->descriptors[1], i2c_dma_cfg->dma_ch, &ch_config));

    /* step 3: set cmd */
    i2c_cfg->cmd_val = I2C_CMD_ISSUE_DATA_TRANSMISSION;
    ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->CMD;
    ch_config.dst_width = DMA_TRANSFER_WIDTH_BYTE;
    ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_cfg->cmd_val);
    ch_config.src_width = DMA_TRANSFER_WIDTH_BYTE;
    ch_config.size_in_byte = 1;
    ch_config.linked_ptr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[3]);
    HPM_CHECK_RET(dma_config_linked_descriptor(i2c_dma_cfg->dma_ptr, &i2c_dma_cfg->descriptors[2], i2c_dma_cfg->dma_ch, &ch_config));

    /* step 4: data transfer */
    ch_config.dst_width = DMA_TRANSFER_WIDTH_BYTE;
    ch_config.src_width = DMA_TRANSFER_WIDTH_BYTE;
    if (read == true) {
        ch_config.dst_addr = core_local_mem_to_sys_address(0, (uint32_t)data);
        ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
        ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
        ch_config.src_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->DATA;
        ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        ch_config.src_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
    } else {
        ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->DATA;
        ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        ch_config.dst_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
        ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)data);
        ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
        ch_config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    }
    ch_config.size_in_byte = data_size;
    ch_config.linked_ptr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_dma_cfg->descriptors[4]);
    HPM_CHECK_RET(dma_config_linked_descriptor(i2c_dma_cfg->dma_ptr, &i2c_dma_cfg->descriptors[3], i2c_dma_cfg->dma_ch, &ch_config));

    /* step 5: restart set addr */
    ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    ch_config.dst_addr = (uint32_t)&i2c_dma_cfg->i2c_ptr->ADDR;
    ch_config.src_addr = core_local_mem_to_sys_address(0, (uint32_t)&i2c_cfg->dev_addr);
    ch_config.size_in_byte = 1;
    ch_config.linked_ptr = NULL;
    HPM_CHECK_RET(dma_config_linked_descriptor(i2c_dma_cfg->dma_ptr, &i2c_dma_cfg->descriptors[4], i2c_dma_cfg->dma_ch, &ch_config));

    return status_success;
}
