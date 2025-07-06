/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mcan.h"
#include "pinmux.h"
#include "slcan.h"
mcan_rx_message_t    g_mcan_rx_frame[CANFD_NUM][MCAN_RXBUF_SIZE_CAN_DEFAULT];
mcan_tx_frame_t      g_mcan_tx_frame[CANFD_NUM];
volatile uint8_t     g_mcan_rx_fifo_complete_flag[CANFD_NUM][MCAN_RXBUF_SIZE_CAN_DEFAULT];

#ifdef  CONFIG_SLCAN0
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct slcan_t slcan0;
#endif

#ifdef  CONFIG_SLCAN1
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct slcan_t slcan1;
#endif

#ifdef  CONFIG_SLCAN2
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct slcan_t slcan2;
#endif

#ifdef  CONFIG_SLCAN3
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct slcan_t slcan3;
#endif

void mcan_isr(struct slcan_t *slcan_port)
{
    MCAN_Type *base = slcan_port->ptr;
    uint32_t flags = mcan_get_interrupt_flags(base);

    /* New message is available in RXFIFO0 */
    if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0) {
        mcan_read_rxfifo(base, 0, (mcan_rx_message_t *)&g_mcan_rx_frame[slcan_port->candev_sn][0]);
        g_mcan_rx_fifo_complete_flag[slcan_port->candev_sn][0] = true;
    }
    /* New message is available in RXFIFO1 */
    if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
        mcan_read_rxfifo(base, 1, (mcan_rx_message_t *) &g_mcan_rx_frame[slcan_port->candev_sn][1]);
        g_mcan_rx_fifo_complete_flag[slcan_port->candev_sn][1] = true;
    }
    /* New message is available in RXBUF */
    if ((flags & MCAN_INT_MSG_STORE_TO_RXBUF) != 0U) {
        /* NOTE: Below code is for demonstration purpose, the performance is not optimized
         *       Users should optimize the performance according to real use case.
         */
        for (uint32_t buf_index = 0; buf_index < MCAN_RXBUF_SIZE_CAN_DEFAULT; buf_index++) {
            if (mcan_is_rxbuf_data_available(base, buf_index)) {
                g_mcan_rx_fifo_complete_flag[slcan_port->candev_sn][buf_index] = true;
                mcan_read_rxbuf(base, buf_index, (mcan_rx_message_t *)&g_mcan_rx_frame[slcan_port->candev_sn][buf_index]);
                mcan_clear_rxbuf_data_available_flag(base, buf_index);
            }
        }
    }
    mcan_clear_interrupt_flags(base, flags);
}

#ifdef  CONFIG_SLCAN0
void mcan0_isr(void)
{
    mcan_isr(&slcan0);
}
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN0, mcan0_isr);
#endif

#ifdef  CONFIG_SLCAN1
void mcan1_isr(void)
{
    mcan_isr(&slcan1);
}
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN1, mcan1_isr);
#endif

#ifdef  CONFIG_SLCAN2
void mcan2_isr(void)
{
    mcan_isr(&slcan2);
}
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN2, mcan2_isr);
#endif

#ifdef  CONFIG_SLCAN3
void mcan3_isr(void)
{
    mcan_isr(&slcan3);
}
SDK_DECLARE_EXT_ISR_M(IRQn_MCAN3, mcan3_isr);
#endif

void mcan_channel_init(uint8_t can_num)
{
    g_mcan_rx_fifo_complete_flag[can_num][0] = false;
    g_mcan_rx_fifo_complete_flag[can_num][1] = false;
}

void mcan_pinmux_init(uint8_t can_num)
{
    switch (can_num) {
#ifdef  CONFIG_SLCAN0
    case 0:
        HPM_IOC->PAD[IOC_PAD_PB00].FUNC_CTL = IOC_PB00_FUNC_CTL_MCAN0_TXD;
        HPM_IOC->PAD[IOC_PAD_PB01].FUNC_CTL = IOC_PB01_FUNC_CTL_MCAN0_RXD; 
        break;
#endif
#ifdef  CONFIG_SLCAN1
    case 1:
        HPM_IOC->PAD[IOC_PAD_PB05].FUNC_CTL = IOC_PB05_FUNC_CTL_MCAN1_TXD;
        HPM_IOC->PAD[IOC_PAD_PB04].FUNC_CTL = IOC_PB04_FUNC_CTL_MCAN1_RXD; 
        break;
#endif
#ifdef  CONFIG_SLCAN2
    case 2:
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_MCAN2_TXD;
        HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_MCAN2_RXD; 
        break;
#endif
#ifdef  CONFIG_SLCAN3
    case 3:
        HPM_IOC->PAD[IOC_PAD_PA15].FUNC_CTL = IOC_PA15_FUNC_CTL_MCAN3_TXD;
        HPM_IOC->PAD[IOC_PAD_PA14].FUNC_CTL = IOC_PA14_FUNC_CTL_MCAN3_RXD;
        break;
#endif
    }

}
