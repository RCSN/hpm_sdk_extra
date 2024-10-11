/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cdc_acm.h"


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
    "HPMicro COMPOSITE DEMO",     /* Product */
    "2024051701",                 /* Serial Number */
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

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[MAX_CDC_COUNT][RX_BUFFER_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[MAX_CDC_COUNT][TX_BUFFER_SIZE];
cdc_device_cfg_t cdc_device[MAX_CDC_COUNT];

uint8_t *usbd_get_write_buffer_ptr(uint8_t uart_num)
{
    return (uint8_t *)write_buffer[uart_num - 1];
}

uint8_t *usbd_get_read_buffer_ptr(uint8_t uart_num)
{
    return (uint8_t *)read_buffer[uart_num - 1];
}

bool usbd_is_tx_busy(uint8_t uart_num)
{
    return cdc_device[uart_num - 1].ep_tx_busy_flag;
}

void usbd_set_tx_busy(uint8_t uart_num)
{
    cdc_device[uart_num - 1].ep_tx_busy_flag = true;
}

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
        for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
            usbd_ep_start_read(busid, i + 1, read_buffer[i], usbd_get_ep_mps(busid, i + 1));
        }
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;

    default:
        break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    usbd_ep_start_read(busid, ep, read_buffer[ep - 1], usbd_get_ep_mps(busid, ep));
    usbd_ep_start_write(busid, (0x80 | ep), read_buffer[ep - 1], nbytes);
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    uint8_t i;
    if (((nbytes % usbd_get_ep_mps(busid, ep)) == 0) && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
        for (i = 0; i < MAX_CDC_COUNT; i++) {
            if (ep == cdc_device[i].cdc_in_ep.ep_addr) {
                cdc_device[i].ep_tx_busy_flag = false;
                break;
            }
        }
    }
}

void cdc_acm_init(uint8_t busid, uint32_t reg_base)
{
    uint8_t i = 0;
    usbd_desc_register(busid, &cdc_descriptor);
    for (i = 0; i < MAX_CDC_COUNT; i++) {
        usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &cdc_device[i].intf0));
        usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &cdc_device[i].intf1));
        cdc_device[i].cdc_out_ep.ep_addr = i + 1;
        cdc_device[i].cdc_in_ep.ep_addr = (0x80 | (i + 1));
        cdc_device[i].cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        cdc_device[i].cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(busid, &cdc_device[i].cdc_out_ep);
        usbd_add_endpoint(busid, &cdc_device[i].cdc_in_ep);
        cdc_device[i].uart_num = i + 1;
        cdc_device[i].is_open = false;
    }

    usbd_initialize(busid, reg_base, usbd_event_handler);
}

void usbd_init_line_coding(uint8_t uart_num, struct cdc_line_coding *line_coding)
{
    cdc_device[uart_num - 1].s_line_coding.dwDTERate = line_coding->dwDTERate;
    cdc_device[uart_num - 1].s_line_coding.bDataBits = line_coding->bDataBits;
    cdc_device[uart_num - 1].s_line_coding.bParityType = line_coding->bParityType;
    cdc_device[uart_num - 1].s_line_coding.bCharFormat = line_coding->bCharFormat;
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (intf == cdc_device[i].intf0.intf_num) {
            cdc_device[i].is_open = true;
            cdc_device[i].s_line_coding.dwDTERate = line_coding->dwDTERate;
            cdc_device[i].s_line_coding.bDataBits = line_coding->bDataBits;
            cdc_device[i].s_line_coding.bParityType = line_coding->bParityType;
            cdc_device[i].s_line_coding.bCharFormat = line_coding->bCharFormat;
            break;
        }
    }
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (intf == cdc_device[i].intf0.intf_num) {
            cdc_device[i].is_open = true;
            line_coding->dwDTERate = cdc_device[i].s_line_coding.dwDTERate;
            line_coding->bDataBits = cdc_device[i].s_line_coding.bDataBits;
            line_coding->bParityType = cdc_device[i].s_line_coding.bParityType;
            line_coding->bCharFormat = cdc_device[i].s_line_coding.bCharFormat;
            break;
        }
    }
}
