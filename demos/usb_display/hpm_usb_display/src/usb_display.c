/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "usbd_core.h"
#include "usbd_graphic.h"

#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_lcdc_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_sysctl_drv.h"

/*!< endpoint address */
#define VENDOR_IN_EP  0x81
#define VENDOR_OUT_EP 0x01
#ifdef CONFIG_USB_HS
#define VENDOR_MAX_MPS 512
#else
#define VENDOR_MAX_MPS 64
#endif

#define LCD BOARD_LCD_BASE
#define LCD_WIDTH BOARD_LCD_WIDTH
#define LCD_HEIGHT BOARD_LCD_HEIGHT
#define CAM_I2C BOARD_CAM_I2C_BASE

#define _CONCAT3(x, y, z) x ## y ## z
#define CONCAT3(x, y, z) _CONCAT3(x, y, z)

#define PIXEL_FORMAT display_pixel_format_rgb565
#define CAMERA_INTERFACE camera_interface_dvp

/*!< config descriptor size */
#define USB_DISPLAY_CONFIG_DESC_SIZ   (9 + 9 + 7 + 7)

struct usbd_interface intf0;
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[VENDOR_MAX_MPS];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[VENDOR_MAX_MPS];
uint8_t lcdc_buffer[BOARD_LCD_WIDTH * BOARD_LCD_HEIGHT * 2];
//__attribute__((section(".noncacheable")));

static uint32_t pix_index = 0;
uint32_t pix_finsh = 0;

void init_lcd(void)
{
    uint8_t layer_index = 0;
    lcdc_config_t config = {0};
    lcdc_layer_config_t layer = {0};

    lcdc_get_default_config(LCD, &config);
    board_panel_para_to_lcdc(&config);
    lcdc_init(LCD, &config);

    lcdc_get_default_layer_config(LCD, &layer, PIXEL_FORMAT, layer_index);

    layer.position_x = 0;
    layer.position_y = 0;
    layer.width = BOARD_LCD_WIDTH;
    layer.height = BOARD_LCD_HEIGHT;
     layer.pixel_format = display_pixel_format_rgb565;
    layer.buffer = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)lcdc_buffer);
    layer.alphablend.src_alpha = 0xF4; /* src */
    layer.alphablend.dst_alpha = 0xF0; /* dst */
    layer.alphablend.src_alpha_op = display_alpha_op_override;
    layer.alphablend.dst_alpha_op = display_alpha_op_override;
    layer.background.u= 0x00000000;
    layer.alphablend.mode = display_alphablend_mode_src_over;

    if (status_success != lcdc_config_layer(LCD, layer_index, &layer, true)) {
        printf("failed to configure layer\n");
        while(1);
    }
}

void usbd_event_handler(uint8_t event)
{
    switch (event) {
    case USBD_EVENT_RESET:
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        /* setup first out ep read transfer */
        usbd_ep_start_read(VENDOR_OUT_EP, read_buffer, VENDOR_MAX_MPS);
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;

    default:
        break;
    }
}

static void usbd_graphic_bulk_out(uint8_t ep, uint32_t nbytes)
{
    uint32_t cmd = *(uint32_t *)read_buffer;
    if (cmd & 0x01) {
        // USB_LOG_RAW("frame start %d %d\n", nbytes, pix_index);
        pix_index = 0;
    }
    memcpy(&lcdc_buffer[pix_index], &read_buffer[4], nbytes - 4);
    pix_index += (nbytes - 4);
    usbd_ep_start_read(ep, read_buffer, VENDOR_MAX_MPS);

    if (cmd & 0x04) {
        // USB_LOG_RAW("frame stop%d\n", pix_index);
        pix_finsh = 1;
    }
    if (cmd & 0x08) {
        // USB_LOG_RAW("frame close \n");
    }

}

static void usbd_graphic_bulk_in(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("in ep:0x%02x actual in len:%d\r\n", ep, nbytes);
}

/*!< endpoint call back */
struct usbd_endpoint graphic_out_ep = {
    .ep_addr = VENDOR_OUT_EP,
    .ep_cb = usbd_graphic_bulk_out
};

struct usbd_endpoint graphic_in_ep = {
    .ep_addr = VENDOR_IN_EP,
    .ep_cb = usbd_graphic_bulk_in
};


/*!< global descriptor */
static const uint8_t vendor_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0000, 0x00),
    USB_CONFIG_DESCRIPTOR_INIT(USB_DISPLAY_CONFIG_DESC_SIZ, 0x01, 0x01, 0xc0, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_IN_EP, 2, VENDOR_MAX_MPS, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_OUT_EP, 2, VENDOR_MAX_MPS, 0),
        /*
     * string0 descriptor
     */
    USB_LANGID_INIT(USBD_LANGID_STRING),
    /*
     * string1 descriptor
     */
    0x10,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'H', 0x00,                  /* wcChar0 */
    'p', 0x00,                  /* wcChar1 */
    'm', 0x00,                  /* wcChar2 */
    'i', 0x00,                  /* wcChar3 */
    'c', 0x00,                  /* wcChar4 */
    'r', 0x00,                  /* wcChar5 */
    'o', 0x00,                  /* wcChar6 */
    /*
     * string2 descriptor
     */
    0x18,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'U', 0x00,                  /* wcChar0 */
    'S', 0x00,                  /* wcChar1 */
    'B', 0x00,                  /* wcChar2 */
    '_', 0x00,                  /* wcChar3 */
    'G', 0x00,                  /* wcChar4 */
    'r', 0x00,                  /* wcChar5 */
    'a', 0x00,                  /* wcChar6 */
    'p', 0x00,                  /* wcChar7 */
    'h', 0x00,                  /* wcChar8 */
    'i', 0x00,                  /* wcChar9 */
    'c', 0x00,                  /* wcChar10 */

    /*
     * string3 descriptor
     */
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

void usb_display_init(void)
{
    board_init_lcd();
    init_lcd();
    lcdc_turn_on_display(LCD);
    usbd_desc_register(vendor_descriptor);
    usbd_add_interface(usbd_graphic_init_intf(&intf0));
    usbd_add_endpoint(&graphic_out_ep);
    usbd_add_endpoint(&graphic_in_ep);
    usbd_initialize();
}

