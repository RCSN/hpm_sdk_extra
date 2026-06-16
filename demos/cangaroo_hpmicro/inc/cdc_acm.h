/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _CDC_ACM_H
#define _CDC_ACM_H

#include "usbd_core.h"
#include "usbd_cdc.h"
#include "chry_ringbuffer.h"

// #define MAX_CDC_COUNT   (((USB_SOC_DCD_MAX_ENDPOINT_COUNT) / 2) - 1)
#define MAX_CDC_COUNT   (4)


#define USB_BUS_ID 0

/*!< endpoint address */
#define CDC_IN_EP   0x81
#define CDC_OUT_EP  0x01
#define CDC_INT_EP  0x88

#define CDC_IN_EP1  0x82
#define CDC_OUT_EP1 0x02
#define CDC_INT_EP1 0x89

#define CDC_IN_EP2  0x83
#define CDC_OUT_EP2 0x03
#define CDC_INT_EP2 0x8a

#define CDC_IN_EP3  0x84
#define CDC_OUT_EP3 0x04
#define CDC_INT_EP3 0x8b

#define CDC_IN_EP4  0x85
#define CDC_OUT_EP4 0x05
#define CDC_INT_EP4 0x8c

#define CDC_IN_EP5  0x86
#define CDC_OUT_EP5 0x06
#define CDC_INT_EP5 0x8d

#define CDC_IN_EP6  0x87
#define CDC_OUT_EP6 0x07
#define CDC_INT_EP6 0x8e

#define CDC_IN_EP7  0x88
#define CDC_OUT_EP7 0x08
#define CDC_INT_EP7 0x8f

extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
#endif
