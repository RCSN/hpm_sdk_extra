/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "usbd_core.h"
#include "usbd_hid.h"

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t fps;
} __attribute__((packed)) usb_display_res;

usb_display_res lcd_res[] = {
//{320,240,120},
// {320,240,120},
// {480,320,120},
// {480,480,120},
// {640,480,120},
{800,480,60},
// {960,540,120},
// {1024,600,60},
// {1920,480,120},
};

uint8_t EDID[]= {
    // /*00000000H:*/0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x11,0x39,0x00,0x00,0x00,0x00,0x00,0x00,
    // /*00000010H:*/0x05,0x16,0x01,0x03,0x6D,0x32,0x1C,0x78,0xEA,0x5E,0xC0,0xA4,0x59,0x4A,0x98,0x25,
    // /*00000020H:*/0x20,0x50,0x54,0x00,0x00,0x00,0xD1,0xC0,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    // /*00000030H:*/0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2c,
    // /*00000040H:*/0x45,0x00,0xF4,0x19,0x11,0x00,0x00,0x1E,0x00,0x00,0x00,0xFF,0x00,0x30,0x30,0x30,
    // /*00000050H:*/0x30,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x0,0xFD,0x00,0x3B,
    // /*00000060H:*/0x3D,0x42,0x44,0x0F,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
    // /*00000070H:*/0x00,0x55,0x53,0x42,0x20,0x4C,0x43,0x44,0x0A,0x20,0x20,0x20,0x20,0x20,0x00,0xEC
    /*00000000H:*/0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x11,0x39,0x00,0x00,0x00,0x00,0x00,0x00,
    /*00000010H:*/0x05,0x16,0x01,0x03,0x6D,0x32,0x1C,0x78,0xEA,0x5E,0xC0,0xA4,0x59,0x4A,0x98,0x25,
    /*00000020H:*/0x20,0x50,0x54,0x00,0x00,0x00,0xD1,0xC0,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    /*00000030H:*/0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,
    /*00000040H:*/0x45,0x00,0xF4,0x19,0x11,0x00,0x00,0x1E,0x00,0x00,0x00,0xFF,0x00,0x30,0x30,0x30,
    /*00000050H:*/0x30,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFD,0x00,0x3B,
    /*00000060H:*/0x3D,0x42,0x44,0x0F,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
    /*00000070H:*/0x00,0x55,0x53,0x42,0x20,0x4C,0x43,0x44,0x0A,0x20,0x20,0x20,0x20,0x20,0x00,0xEC
};

uint8_t format = 0;

static int graphic_class_interface_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_DBG("graphic Class request: "
                "bRequest 0x%02x\r\n",
                setup->bRequest);

    switch (setup->bRequest) {
        case 0: /* request lcd resolution */
            (*len) = sizeof(lcd_res);
            memcpy((*data), (uint8_t *)lcd_res, (*len));
            break;

        case 1: /* requset edid */
            (*len) = sizeof(EDID);
            memcpy((*data), (uint8_t *)EDID, (*len));
            break;
        
        case 2:/* requset lcd format */
            (*len) = 1;
            (*data)[0] = format;
            break;

        default:
            USB_LOG_WRN("Unhandled graphic Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

struct usbd_interface *usbd_graphic_init_intf(struct usbd_interface *intf)
{
    intf->class_interface_handler = NULL;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = graphic_class_interface_request_handler;
    intf->notify_handler = NULL;

    return intf;
}