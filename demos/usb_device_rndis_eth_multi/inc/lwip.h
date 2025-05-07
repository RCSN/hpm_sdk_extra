/*
 * Copyright (c) 2024-2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef LWIP_H
#define LWIP_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "hpm_enet_drv.h"
#include "hpm_l1c_drv.h"
#include "lwipopts.h"

#define ENET_TX_BUFF_SIZE   (1536U)
#define ENET_RX_BUFF_SIZE   (1536U)

#if (USE_ENET_PORT_COUNT == 1)
#define ENET_TX_BUFF_COUNT  (5U)
#define ENET_RX_BUFF_COUNT  (10U)
/* Exported Macros------------------------------------------------------------*/
#if defined(RGMII) && RGMII
#define ENET_INF_TYPE       enet_inf_rgmii
#define ENET                BOARD_ENET_RGMII
#elif defined(RMII) && RMII
#define ENET_INF_TYPE       enet_inf_rmii
#define ENET                BOARD_ENET_RMII
#endif

/* Exported Variables ------------------------------------------------------*/
extern enet_desc_t desc;
extern uint8_t mac[];
#elif (USE_ENET_PORT_COUNT == 2)
#define ENET_TX_BUFF_COUNT  (10U)
#define ENET_RX_BUFF_COUNT  (20U)

typedef ENET_Type enet_base_t;

/* Exported Macros------------------------------------------------------------*/
#if defined(RGMII) && RGMII
#define ENET_INF_TYPE       enet_inf_rgmii
#define ENET                BOARD_ENET_RGMII
#else
#define ENET_INF_TYPE       enet_inf_rmii
#define ENET                BOARD_ENET_RMII
#endif

/* Exported Variables ------------------------------------------------------*/
extern enet_desc_t desc[];
extern uint8_t mac[BOARD_ENET_COUNT][ENET_MAC];

#if __ENABLE_ENET_RECEIVE_INTERRUPT
extern volatile bool rx_flag[];
#endif
#endif

#endif /* LWIP_H */
