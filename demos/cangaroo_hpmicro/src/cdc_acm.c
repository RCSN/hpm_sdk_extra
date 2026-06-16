/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cdc_acm.h"
#include "slcan.h"

/*!< config descriptor size 1cdc = 2 interface(bilateral(in + out) + int)*/

#define CDC_DATA_HS_MAX_PACKET_SIZE                 512U  /* Endpoint IN & OUT Packet size */
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U  /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SIZE                         8U  /* Control Endpoint Packet size */
#define CDC_HS_BINTERVAL                            0x10U
#define USB_CONFIG_SIZE (9 + (CDC_ACM_DESCRIPTOR_LEN * MAX_CDC_COUNT))

#define RX_BUFFER_SIZE                             2048U
#define TX_BUFFER_SIZE                             2048U

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor_hs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02 * MAX_CDC_COUNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_INT_EP1, CDC_OUT_EP1, CDC_IN_EP1, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_INT_EP2, CDC_OUT_EP2, CDC_IN_EP2, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_INT_EP3, CDC_OUT_EP3, CDC_IN_EP3, USB_BULK_EP_MPS_HS, 0x00),
#if (MAX_CDC_COUNT > 4)
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_INT_EP4, CDC_OUT_EP4, CDC_IN_EP4, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0a, CDC_INT_EP5, CDC_OUT_EP5, CDC_IN_EP5, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0c, CDC_INT_EP6, CDC_OUT_EP6, CDC_IN_EP6, USB_BULK_EP_MPS_HS, 0x00),
#endif
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02 * MAX_CDC_COUNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_IN_EP1, CDC_OUT_EP1, CDC_IN_EP1, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_IN_EP2, CDC_OUT_EP2, CDC_IN_EP2, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_IN_EP3, CDC_OUT_EP3, CDC_IN_EP3, USB_BULK_EP_MPS_FS, 0x00),
#if (MAX_CDC_COUNT > 4)
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_IN_EP4, CDC_OUT_EP4, CDC_IN_EP4, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0a, CDC_IN_EP5, CDC_OUT_EP5, CDC_IN_EP5, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0c, CDC_IN_EP6, CDC_OUT_EP6, CDC_IN_EP6, USB_BULK_EP_MPS_FS, 0x00),
#endif
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02 * MAX_CDC_COUNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_IN_EP1, CDC_OUT_EP1, CDC_IN_EP1, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_IN_EP2, CDC_OUT_EP2, CDC_IN_EP2, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_IN_EP3, CDC_OUT_EP3, CDC_IN_EP3, USB_BULK_EP_MPS_FS, 0x00),
#if (MAX_CDC_COUNT > 4)
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_IN_EP4, CDC_OUT_EP4, CDC_IN_EP4, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0a, CDC_IN_EP5, CDC_OUT_EP5, CDC_IN_EP5, USB_BULK_EP_MPS_FS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0c, CDC_IN_EP6, CDC_OUT_EP6, CDC_IN_EP6, USB_BULK_EP_MPS_FS, 0x00),
#endif
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02 * MAX_CDC_COUNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_IN_EP1, CDC_OUT_EP1, CDC_IN_EP1, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_IN_EP2, CDC_OUT_EP2, CDC_IN_EP2, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_IN_EP3, CDC_OUT_EP3, CDC_IN_EP3, USB_BULK_EP_MPS_HS, 0x00),
#if (MAX_CDC_COUNT > 4)
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_IN_EP4, CDC_OUT_EP4, CDC_IN_EP4, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0a, CDC_IN_EP5, CDC_OUT_EP5, CDC_IN_EP5, USB_BULK_EP_MPS_HS, 0x00),
    CDC_ACM_DESCRIPTOR_INIT(0x0c, CDC_IN_EP6, CDC_OUT_EP6, CDC_IN_EP6, USB_BULK_EP_MPS_HS, 0x00),
#endif
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* Langid */
    "HPMicro",                    /* Manufacturer */
    "USB Virtual COM",     /* Product */
    "A02024030801",                 /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return config_descriptor_hs;
    } else if (speed == USB_SPEED_FULL) {
        return config_descriptor_fs;
    } else {
        return NULL;
    }
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;

    return device_quality_descriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    if (speed == USB_SPEED_HIGH) {
        return other_speed_config_descriptor_hs;
    } else if (speed == USB_SPEED_FULL) {
        return other_speed_config_descriptor_fs;
    } else {
        return NULL;
    }
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
};

static void usbd_event_handler(uint8_t busid, uint8_t event)
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
#ifdef CONFIG_SLCAN0
        usbd_ep_start_read(0, CDC_OUT_EP, slcan0.g_cdc_can_device.read_buffer, usbd_get_ep_mps(busid, CDC_OUT_EP));
#endif
#ifdef CONFIG_SLCAN1
        usbd_ep_start_read(0, CDC_OUT_EP1, slcan1.g_cdc_can_device.read_buffer, usbd_get_ep_mps(busid, CDC_OUT_EP1));
#endif
#ifdef CONFIG_SLCAN2
        usbd_ep_start_read(0, CDC_OUT_EP2, slcan2.g_cdc_can_device.read_buffer, usbd_get_ep_mps(busid, CDC_OUT_EP2));
#endif
#ifdef CONFIG_SLCAN3
        usbd_ep_start_read(0, CDC_OUT_EP3, slcan3.g_cdc_can_device.read_buffer, usbd_get_ep_mps(busid, CDC_OUT_EP3));
#endif
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;

    default:
        break;
    }
}


void cdc_acm_init(uint8_t busid, uint32_t reg_base)
{
    uint8_t i = 0;
    struct cdc_line_coding line_coding;
    usbd_desc_register(busid, &cdc_descriptor);
    cdc_acm_can_init();
    usbd_initialize(busid, reg_base, usbd_event_handler);
}


void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
#ifdef CONFIG_SLCAN0
    if (intf == slcan0.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan0.g_cdc_can_device.cdc_device.is_open = true;
        slcan0.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
        slcan0.g_cdc_can_device.cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
        slcan0.g_cdc_can_device.cdc_device.s_line_coding.bParityType = line_coding->bParityType;
        slcan0.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
    }
#endif 
#ifdef CONFIG_SLCAN1
    if (intf == slcan1.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan1.g_cdc_can_device.cdc_device.is_open = true;
        slcan1.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
        slcan1.g_cdc_can_device.cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
        slcan1.g_cdc_can_device.cdc_device.s_line_coding.bParityType = line_coding->bParityType;
        slcan1.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
    }
#endif 
#ifdef CONFIG_SLCAN2
    if (intf == slcan2.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan2.g_cdc_can_device.cdc_device.is_open = true;
        slcan2.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
        slcan2.g_cdc_can_device.cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
        slcan2.g_cdc_can_device.cdc_device.s_line_coding.bParityType = line_coding->bParityType;
        slcan2.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
    } 
#endif 
#ifdef CONFIG_SLCAN3
    if (intf == slcan3.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan3.g_cdc_can_device.cdc_device.is_open = true;
        slcan3.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
        slcan3.g_cdc_can_device.cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
        slcan3.g_cdc_can_device.cdc_device.s_line_coding.bParityType = line_coding->bParityType;
        slcan3.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
    } 
#endif 
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
#ifdef CONFIG_SLCAN0
    if (intf == slcan0.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan0.g_cdc_can_device.cdc_device.is_open = true;
        line_coding->dwDTERate = slcan0.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate;
        line_coding->bDataBits = slcan0.g_cdc_can_device.cdc_device.s_line_coding.bDataBits;
        line_coding->bParityType = slcan0.g_cdc_can_device.cdc_device.s_line_coding.bParityType;
        line_coding->bCharFormat = slcan0.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat;
    }
#endif 
#ifdef CONFIG_SLCAN1
    if (intf == slcan1.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan1.g_cdc_can_device.cdc_device.is_open = true;
        line_coding->dwDTERate = slcan1.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate;
        line_coding->bDataBits = slcan1.g_cdc_can_device.cdc_device.s_line_coding.bDataBits;
        line_coding->bParityType = slcan1.g_cdc_can_device.cdc_device.s_line_coding.bParityType;
        line_coding->bCharFormat = slcan1.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat;
    }
#endif 
#ifdef CONFIG_SLCAN2
    if (intf == slcan2.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan2.g_cdc_can_device.cdc_device.is_open = true;
        line_coding->dwDTERate = slcan2.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate;
        line_coding->bDataBits = slcan2.g_cdc_can_device.cdc_device.s_line_coding.bDataBits;
        line_coding->bParityType = slcan2.g_cdc_can_device.cdc_device.s_line_coding.bParityType;
        line_coding->bCharFormat = slcan2.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat;
    }
#endif 
#ifdef CONFIG_SLCAN3
    if (intf == slcan3.g_cdc_can_device.cdc_device.intf0.intf_num) {
        slcan3.g_cdc_can_device.cdc_device.is_open = true;
        line_coding->dwDTERate = slcan3.g_cdc_can_device.cdc_device.s_line_coding.dwDTERate;
        line_coding->bDataBits = slcan3.g_cdc_can_device.cdc_device.s_line_coding.bDataBits;
        line_coding->bParityType = slcan3.g_cdc_can_device.cdc_device.s_line_coding.bParityType;
        line_coding->bCharFormat = slcan3.g_cdc_can_device.cdc_device.s_line_coding.bCharFormat;
    }
#endif
}



