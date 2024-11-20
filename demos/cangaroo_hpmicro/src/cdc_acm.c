/*
 * Copyright (c) 2022-2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cdc_acm.h"
#include "chry_ringbuffer.h"

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

typedef struct {
    uint8_t can_num;
    cdc_device_cfg_t cdc_device;
    uint8_t read_buffer[RX_BUFFER_SIZE];
    uint8_t write_buffer[RX_BUFFER_SIZE];
    chry_ringbuffer_t usb_out_rb;
    uint8_t usb_out_mempool[1024];
} cdc_can_device_t;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX cdc_can_device_t g_cdc_can_device[MAX_CDC_COUNT];

// cdc_device_cfg_t cdc_device[MAX_CDC_COUNT];

uint8_t *usbd_get_write_buffer_ptr(uint8_t can_num)
{
    return (uint8_t *)g_cdc_can_device[can_num].write_buffer;
}

uint8_t *usbd_get_read_buffer_ptr(uint8_t can_num)
{
    return (uint8_t *)g_cdc_can_device[can_num].read_buffer;
}

bool usbd_is_tx_busy(uint8_t can_num)
{
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (g_cdc_can_device[i].can_num == can_num) {
            return g_cdc_can_device[i].cdc_device.ep_tx_busy_flag;
        }
    }
    return true;
}

void usbd_set_tx_busy(uint8_t can_num)
{
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (g_cdc_can_device[i].can_num == can_num) {
            g_cdc_can_device[i].cdc_device.ep_tx_busy_flag = true;
        }
    }
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
            usbd_ep_start_read(busid, i + 1, g_cdc_can_device[i].read_buffer, usbd_get_ep_mps(busid, i + 1));
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
    uint16_t len = 0;
    uint8_t can_num = (ep - 1);
    // printf("ep:%d\n", ep);
    // for (uint32_t i = 0; i < nbytes; i++) {
    //     printf("%c", g_cdc_can_device[ep - 1].read_buffer[i]);
    // }
    if ((g_cdc_can_device[ep - 1].read_buffer[0] == 'r') && (g_cdc_can_device[ep - 1].read_buffer[1] == '_') && (g_cdc_can_device[ep - 1].read_buffer[2] == 'c') &&
        (g_cdc_can_device[ep - 1].read_buffer[3] == 'a') && (g_cdc_can_device[ep - 1].read_buffer[4] == 'n')) {
            can_num = g_cdc_can_device[ep - 1].read_buffer[5] - '0';
            g_cdc_can_device[ep - 1].can_num = can_num;
            len = sprintf((char *)g_cdc_can_device[ep - 1].write_buffer, "HPM_CAN%d_BUS", can_num);
            // printf("%s    %d\r\n ", g_cdc_can_device[ep - 1].write_buffer, can_num);
            usbd_ep_start_write(busid, 0x80 | ep, (const uint8_t *)g_cdc_can_device[ep - 1].write_buffer, len);
    } else {
        chry_ringbuffer_write(&g_cdc_can_device[ep - 1].usb_out_rb, g_cdc_can_device[ep - 1].read_buffer, nbytes);
    }
    usbd_ep_start_read(busid, ep, g_cdc_can_device[ep - 1].read_buffer, usbd_get_ep_mps(busid, ep));
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    uint8_t i;
    if (((nbytes % usbd_get_ep_mps(busid, ep)) == 0) && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
        for (i = 0; i < MAX_CDC_COUNT; i++) {
            if (ep == g_cdc_can_device[i].cdc_device.cdc_in_ep.ep_addr) {
                g_cdc_can_device[i].cdc_device.ep_tx_busy_flag = false;
                break;
            }
        }
    }
}

void cdc_acm_init(uint8_t busid, uint32_t reg_base)
{
    uint8_t i = 0;
    struct cdc_line_coding line_coding;
    usbd_desc_register(busid, &cdc_descriptor);
    for (i = 0; i < MAX_CDC_COUNT; i++) {
        usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &g_cdc_can_device[i].cdc_device.intf0));
        usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &g_cdc_can_device[i].cdc_device.intf1));
        g_cdc_can_device[i].cdc_device.cdc_out_ep.ep_addr = i + 1;
        g_cdc_can_device[i].cdc_device.cdc_in_ep.ep_addr = (0x80 | (i + 1));
        g_cdc_can_device[i].cdc_device.cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        g_cdc_can_device[i].cdc_device.cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(busid, &g_cdc_can_device[i].cdc_device.cdc_out_ep);
        usbd_add_endpoint(busid, &g_cdc_can_device[i].cdc_device.cdc_in_ep);
        g_cdc_can_device[i].cdc_device.is_open = false;
        if (0 == chry_ringbuffer_init(&g_cdc_can_device[i].usb_out_rb, g_cdc_can_device[i].usb_out_mempool, sizeof(g_cdc_can_device[i].usb_out_mempool))) {
            printf("chry_ringbuffer_init success %d\r\n", i);
        } else {
            printf("chry_ringbuffer_init error %d\r\n", i);
        }
        line_coding.dwDTERate = 1000000;
        line_coding.bDataBits = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = 0;
        usbd_init_line_coding(i, &line_coding);
    }

    usbd_initialize(busid, reg_base, usbd_event_handler);
}

void usbd_init_line_coding(uint8_t can_num, struct cdc_line_coding *line_coding)
{
    g_cdc_can_device[can_num].cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
    g_cdc_can_device[can_num].cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
    g_cdc_can_device[can_num].cdc_device.s_line_coding.bParityType = line_coding->bParityType;
    g_cdc_can_device[can_num].cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (intf == g_cdc_can_device[i].cdc_device.intf0.intf_num) {
            g_cdc_can_device[i].cdc_device.is_open = true;
            g_cdc_can_device[i].cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
            g_cdc_can_device[i].cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
            g_cdc_can_device[i].cdc_device.s_line_coding.bParityType = line_coding->bParityType;
            g_cdc_can_device[i].cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
            break;
        }
    }
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (intf == g_cdc_can_device[i].cdc_device.intf0.intf_num) {
            g_cdc_can_device[i].cdc_device.is_open = true;
            line_coding->dwDTERate = g_cdc_can_device[i].cdc_device.s_line_coding.dwDTERate;
            line_coding->bDataBits = g_cdc_can_device[i].cdc_device.s_line_coding.bDataBits;
            line_coding->bParityType = g_cdc_can_device[i].cdc_device.s_line_coding.bParityType;
            line_coding->bCharFormat = g_cdc_can_device[i].cdc_device.s_line_coding.bCharFormat;
            break;
        }
    }
}

bool get_usb_out_char(uint8_t can_num, uint8_t *data)
{
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (g_cdc_can_device[i].can_num == can_num) {
            return chry_ringbuffer_read_byte(&g_cdc_can_device[i].usb_out_rb, data);
        }
    }
    return -1;
}

bool get_usb_out_is_empty(uint8_t can_num)
{
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (g_cdc_can_device[i].can_num == can_num) {
            return chry_ringbuffer_check_empty(&g_cdc_can_device[i].usb_out_rb);
        }
    }
    return true;
}

uint32_t write_usb_data(uint8_t can_num, uint8_t *data, uint32_t len)
{
    uint32_t res;
    for (uint8_t i = 0; i < MAX_CDC_COUNT; i++) {
        if (g_cdc_can_device[i].can_num == can_num) {
            if (usbd_is_tx_busy(can_num) == false) {
                usbd_set_tx_busy(can_num);
                res = usbd_ep_start_write(USB_BUS_ID, g_cdc_can_device[i].cdc_device.cdc_in_ep.ep_addr, data, len);
                if (res != 0) {
                    return 0;
                }
            } else {
                return 0;
            }
            return len;
        }
    }
    return 0;
}



