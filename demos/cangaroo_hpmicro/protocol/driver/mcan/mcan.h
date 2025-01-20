/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __MCAN_H__
#define __MCAN_H__
#include "hpm_mcan_drv.h"

#define CANFD_NUM           (4U)

extern mcan_rx_message_t    g_mcan_rx_frame[CANFD_NUM][MCAN_RXBUF_SIZE_CAN_DEFAULT];
extern mcan_tx_frame_t      g_mcan_tx_frame[CANFD_NUM];
extern volatile uint8_t     g_mcan_rx_fifo_complete_flag[CANFD_NUM][MCAN_RXBUF_SIZE_CAN_DEFAULT];


void mcan_channel_init(uint8_t can_num);
void mcan_pinmux_init(uint8_t can_num);
#endif
