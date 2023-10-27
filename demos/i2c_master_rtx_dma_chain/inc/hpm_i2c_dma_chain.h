/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "hpm_clock_drv.h"
#include "hpm_i2c_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"

typedef struct {
  uint32_t  dev_addr;
  uint32_t ctrl_val;
  uint32_t setup_val;
  uint32_t cmd_val;
} hpm_i2c_cfg_t;


typedef struct {
    I2C_Type *i2c_ptr;
    DMA_Type *dma_ptr;
    uint8_t dma_ch;
    dma_linked_descriptor_t *descriptors;
} hpm_i2c_dma_chain_cfg_t;

hpm_stat_t hpm_i2c_master_transfer_dma_chain_start(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, hpm_i2c_cfg_t *i2c_cfg, bool read);
hpm_stat_t hpm_i2c_master_transfer_dma_chain_setup(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, hpm_i2c_cfg_t *i2c_cfg, bool read);
hpm_stat_t hpm_i2c_master_transfer_dma_chain_generate(hpm_i2c_dma_chain_cfg_t *i2c_dma_cfg, uint8_t *data, uint16_t data_size, hpm_i2c_cfg_t *i2c_cfg, bool read);



