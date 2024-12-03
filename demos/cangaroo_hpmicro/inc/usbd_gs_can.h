/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBD_GS_CAN_H
#define USBD_GS_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

struct usbd_interface *usbd_gs_can_init_intf(uint8_t busid, struct usbd_interface *intf);

#ifdef __cplusplus
}
#endif

#endif /* USBD_GS_CAN_H */