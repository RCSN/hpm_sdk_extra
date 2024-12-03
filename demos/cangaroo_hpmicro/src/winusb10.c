/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "usbd_core.h"
#include "usbd_gs_can.h"
#include "gs_usb.h"

#define GS_CAN_NUM              4

#define USBD_GS_CAN_VENDOR_CODE 0x20

#ifdef CONFIG_CANFD
#define CAN_DATA_MAX_PACKET_SIZE 64 /* Endpoint IN & OUT Packet size */
#else
#define CAN_DATA_MAX_PACKET_SIZE 32 /* Endpoint IN & OUT Packet size */
#endif

#define GSUSB_ENDPOINT_IN1  0x81
#define GSUSB_ENDPOINT_OUT1 0x02

#define GSUSB_ENDPOINT_IN2  0x83
#define GSUSB_ENDPOINT_OUT2 0x04

#define GSUSB_ENDPOINT_IN3  0x85
#define GSUSB_ENDPOINT_OUT3 0x06

#define GSUSB_ENDPOINT_IN4  0x87
#define GSUSB_ENDPOINT_OUT4 0x08

#define USBD_VID_WINUSB     0x1d50
#define USBD_PID_WINUSB     0x606f
#define USB_CONFIG_SIZE     (9 + (9 + 7 + 7) * GS_CAN_NUM)

static const uint8_t WCID_StringDescriptor_MSOS[18] = {
    /*
     * MS OS string descriptor
     */
    0x12,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    /* MSFT100 */
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, /* wcChar_7 */
    '1', 0x00, '0', 0x00, '0', 0x00,            /* wcChar_7 */
    USBD_GS_CAN_VENDOR_CODE,                    /* bVendorCode */
    0x00,                                       /* bReserved */
};

static const uint8_t WINUSB_WCIDDescriptor[] = {
    /*
     * WCID descriptor
     */
    0x10 + GS_CAN_NUM * 0x18, 0x00, 0x00, 0x00, /* dwLength */
    0x00, 0x01,                               /* bcdVersion */
    0x04, 0x00,                               /* wIndex */
    GS_CAN_NUM,                               /* bCount */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* bReserved_7 */

    /*
     * WCID function descriptor
     */
    0x00, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* WINUSB */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, /* cCID_8 */
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
#if GS_CAN_NUM > 1
    /*
     * WCID function descriptor
     */
    0x01, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* WINUSB */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, /* cCID_8 */
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
#endif
#if GS_CAN_NUM > 2
    /*
     * WCID function descriptor
     */
    0x02, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* WINUSB */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, /* cCID_8 */
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
#endif
#if GS_CAN_NUM > 3
    /*
     * WCID function descriptor
     */
    0x03, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* WINUSB */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, /* cCID_8 */
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
#endif
};

static const uint8_t WINUSB_IF0_WCIDProperties[] = {
    0x92, 0x00, 0x00, 0x00, /* length */
    0x00, 0x01,             /* version 1.0 */
    0x05, 0x00,             /* descr index (0x0005) */
    0x01, 0x00,             /* number of sections */
    0x88, 0x00, 0x00, 0x00, /* property section size */
    0x07, 0x00, 0x00, 0x00, /* property data type 7: Unicode REG_MULTI_SZ */
    0x2a, 0x00,             /* property name length */

    0x44, 0x00, 0x65, 0x00, /* property name "DeviceInterfaceGUIDs" */
    0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x47, 0x00, 0x55, 0x00, 0x49, 0x00, 0x44, 0x00, 0x73, 0x00, 0x00, 0x00,

    0x50, 0x00, 0x00, 0x00, /* property data length */

    0x7b, 0x00, 0x63, 0x00, /* property name: "{c15b4308-04d3-11e6-b3ea-6057189e6443}\0\0" */
    0x31, 0x00, 0x35, 0x00, 0x62, 0x00, 0x34, 0x00, 0x33, 0x00, 0x30, 0x00, 0x38, 0x00, 0x2d, 0x00, 0x30, 0x00, 0x34, 0x00, 0x64, 0x00, 0x33, 0x00, 0x2d, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x65, 0x00, 0x36, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x33, 0x00, 0x65, 0x00, 0x61, 0x00, 0x2d, 0x00, 0x36, 0x00, 0x30, 0x00, 0x35, 0x00,
    0x37, 0x00, 0x31, 0x00, 0x38, 0x00, 0x39, 0x00, 0x65, 0x00, 0x36, 0x00, 0x34, 0x00, 0x34, 0x00, 0x33, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t WINUSB_IF1_WCIDProperties[] = {
    0x92, 0x00, 0x00, 0x00, /* length */
    0x00, 0x01,             /* version 1.0 */
    0x05, 0x00,             /* descr index (0x0005) */
    0x01, 0x00,             /* number of sections */
    0x88, 0x00, 0x00, 0x00, /* property section size */
    0x07, 0x00, 0x00, 0x00, /* property data type 7: Unicode REG_MULTI_SZ */
    0x2a, 0x00,             /* property name length */

    0x44, 0x00, 0x65, 0x00, /* property name "DeviceInterfaceGUIDs" */
    0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x47, 0x00, 0x55, 0x00, 0x49, 0x00, 0x44, 0x00, 0x73, 0x00, 0x00, 0x00,

    0x50, 0x00, 0x00, 0x00, /* property data length */

    0x7b, 0x00, 0x63, 0x00, /* property name: "{c15b4308-04d3-11e6-b3ea-6057189e6443}\0\0" */
    0x31, 0x00, 0x35, 0x00, 0x62, 0x00, 0x34, 0x00, 0x33, 0x00, 0x30, 0x00, 0x38, 0x00, 0x2d, 0x00, 0x30, 0x00, 0x34, 0x00, 0x64, 0x00, 0x33, 0x00, 0x2d, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x65, 0x00, 0x36, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x33, 0x00, 0x65, 0x00, 0x61, 0x00, 0x2d, 0x00, 0x36, 0x00, 0x30, 0x00, 0x35, 0x00,
    0x37, 0x00, 0x31, 0x00, 0x38, 0x00, 0x39, 0x00, 0x65, 0x00, 0x36, 0x00, 0x34, 0x00, 0x34, 0x00, 0x33, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t WINUSB_IF2_WCIDProperties[] = {
    0x92, 0x00, 0x00, 0x00, /* length */
    0x00, 0x01,             /* version 1.0 */
    0x05, 0x00,             /* descr index (0x0005) */
    0x01, 0x00,             /* number of sections */
    0x88, 0x00, 0x00, 0x00, /* property section size */
    0x07, 0x00, 0x00, 0x00, /* property data type 7: Unicode REG_MULTI_SZ */
    0x2a, 0x00,             /* property name length */

    0x44, 0x00, 0x65, 0x00, /* property name "DeviceInterfaceGUIDs" */
    0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x47, 0x00, 0x55, 0x00, 0x49, 0x00, 0x44, 0x00, 0x73, 0x00, 0x00, 0x00,

    0x50, 0x00, 0x00, 0x00, /* property data length */

    0x7b, 0x00, 0x63, 0x00, /* property name: "{c15b4308-04d3-11e6-b3ea-6057189e6443}\0\0" */
    0x31, 0x00, 0x35, 0x00, 0x62, 0x00, 0x34, 0x00, 0x33, 0x00, 0x30, 0x00, 0x38, 0x00, 0x2d, 0x00, 0x30, 0x00, 0x34, 0x00, 0x64, 0x00, 0x33, 0x00, 0x2d, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x65, 0x00, 0x36, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x33, 0x00, 0x65, 0x00, 0x61, 0x00, 0x2d, 0x00, 0x36, 0x00, 0x30, 0x00, 0x35, 0x00,
    0x37, 0x00, 0x31, 0x00, 0x38, 0x00, 0x39, 0x00, 0x65, 0x00, 0x36, 0x00, 0x34, 0x00, 0x34, 0x00, 0x33, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t WINUSB_IF3_WCIDProperties[] = {
    0x92, 0x00, 0x00, 0x00, /* length */
    0x00, 0x01,             /* version 1.0 */
    0x05, 0x00,             /* descr index (0x0005) */
    0x01, 0x00,             /* number of sections */
    0x88, 0x00, 0x00, 0x00, /* property section size */
    0x07, 0x00, 0x00, 0x00, /* property data type 7: Unicode REG_MULTI_SZ */
    0x2a, 0x00,             /* property name length */

    0x44, 0x00, 0x65, 0x00, /* property name "DeviceInterfaceGUIDs" */
    0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x47, 0x00, 0x55, 0x00, 0x49, 0x00, 0x44, 0x00, 0x73, 0x00, 0x00, 0x00,

    0x50, 0x00, 0x00, 0x00, /* property data length */

    0x7b, 0x00, 0x63, 0x00, /* property name: "{c15b4308-04d3-11e6-b3ea-6057189e6443}\0\0" */
    0x31, 0x00, 0x35, 0x00, 0x62, 0x00, 0x34, 0x00, 0x33, 0x00, 0x30, 0x00, 0x38, 0x00, 0x2d, 0x00, 0x30, 0x00, 0x34, 0x00, 0x64, 0x00, 0x33, 0x00, 0x2d, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x65, 0x00, 0x36, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x33, 0x00, 0x65, 0x00, 0x61, 0x00, 0x2d, 0x00, 0x36, 0x00, 0x30, 0x00, 0x35, 0x00,
    0x37, 0x00, 0x31, 0x00, 0x38, 0x00, 0x39, 0x00, 0x65, 0x00, 0x36, 0x00, 0x34, 0x00, 0x34, 0x00, 0x33, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t *WINUSB_IFx_WCIDProperties[] = {
    WINUSB_IF0_WCIDProperties,
#if GS_CAN_NUM > 1
    WINUSB_IF1_WCIDProperties,
#endif
#if GS_CAN_NUM > 2
    WINUSB_IF2_WCIDProperties,
#endif
#if GS_CAN_NUM > 3
    WINUSB_IF3_WCIDProperties,
#endif
};

struct usb_msosv1_descriptor msosv1_desc = {
    .string = WCID_StringDescriptor_MSOS,
    .vendor_code = USBD_GS_CAN_VENDOR_CODE,
    .compat_id = WINUSB_WCIDDescriptor,
    .comp_id_property = WINUSB_IFx_WCIDProperties,
};

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID_WINUSB, USBD_PID_WINUSB, 0x0000, 0x01),
};

static const uint8_t config_descriptor_hs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, GS_CAN_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0xff, 0x00, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#if GS_CAN_NUM > 1
    USB_INTERFACE_DESCRIPTOR_INIT(0x01, 0x00, 0x02, 0xff, 0xff, 0x00, 0x05),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 2
    USB_INTERFACE_DESCRIPTOR_INIT(0x02, 0x00, 0x02, 0xff, 0xff, 0x00, 0x06),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 3
    USB_INTERFACE_DESCRIPTOR_INIT(0x03, 0x00, 0x02, 0xff, 0xff, 0x00, 0x07),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, GS_CAN_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0xff, 0x00, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#if GS_CAN_NUM > 1
    USB_INTERFACE_DESCRIPTOR_INIT(0x01, 0x00, 0x02, 0xff, 0xff, 0x00, 0x05),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 2
    USB_INTERFACE_DESCRIPTOR_INIT(0x02, 0x00, 0x02, 0xff, 0xff, 0x00, 0x06),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 3
    USB_INTERFACE_DESCRIPTOR_INIT(0x03, 0x00, 0x02, 0xff, 0xff, 0x00, 0x07),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, GS_CAN_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0xff, 0x00, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#if GS_CAN_NUM > 1
    USB_INTERFACE_DESCRIPTOR_INIT(0x01, 0x00, 0x02, 0xff, 0xff, 0x00, 0x05),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 2
    USB_INTERFACE_DESCRIPTOR_INIT(0x02, 0x00, 0x02, 0xff, 0xff, 0x00, 0x06),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 3
    USB_INTERFACE_DESCRIPTOR_INIT(0x03, 0x00, 0x02, 0xff, 0xff, 0x00, 0x07),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, GS_CAN_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0xff, 0x00, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT1, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#if GS_CAN_NUM > 1
    USB_INTERFACE_DESCRIPTOR_INIT(0x01, 0x00, 0x02, 0xff, 0xff, 0x00, 0x05),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT2, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 2
    USB_INTERFACE_DESCRIPTOR_INIT(0x02, 0x00, 0x02, 0xff, 0xff, 0x00, 0x06),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT3, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
#if GS_CAN_NUM > 3
    USB_INTERFACE_DESCRIPTOR_INIT(0x03, 0x00, 0x02, 0xff, 0xff, 0x00, 0x07),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_IN4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(GSUSB_ENDPOINT_OUT4, USB_ENDPOINT_TYPE_BULK, CAN_DATA_MAX_PACKET_SIZE, 0x00),
#endif
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },      /* Langid */
    "HPMicro",                         /* Manufacturer */
    "candleLight USB to CAN adapter",  /* Product */
    "2024051701",                      /* Serial Number */
    "candleLight USB to CAN adapter0", /* intf */
    "candleLight USB to CAN adapter1", /* intf */
    "candleLight USB to CAN adapter2", /* intf */
    "candleLight USB to CAN adapter3", /* intf */
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

const struct usb_descriptor winusb_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
    .msosv1_descriptor = &msosv1_desc,
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
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_winusb_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
}

void usbd_winusb_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("actual in len:%d\r\n", nbytes);

    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
    }
}

struct usbd_endpoint winusb_out_ep1 = {
    .ep_addr = GSUSB_ENDPOINT_OUT1,
    .ep_cb = usbd_winusb_out
};

struct usbd_endpoint winusb_in_ep1 = {
    .ep_addr = GSUSB_ENDPOINT_IN1,
    .ep_cb = usbd_winusb_in
};

#if GS_CAN_NUM > 1
struct usbd_endpoint winusb_out_ep2 = {
    .ep_addr = GSUSB_ENDPOINT_OUT2,
    .ep_cb = usbd_winusb_out
};

struct usbd_endpoint winusb_in_ep2 = {
    .ep_addr = GSUSB_ENDPOINT_IN2,
    .ep_cb = usbd_winusb_in
};
#endif

#if GS_CAN_NUM > 2
struct usbd_endpoint winusb_out_ep3 = {
    .ep_addr = GSUSB_ENDPOINT_OUT3,
    .ep_cb = usbd_winusb_out
};

struct usbd_endpoint winusb_in_ep3 = {
    .ep_addr = GSUSB_ENDPOINT_IN3,
    .ep_cb = usbd_winusb_in
};
#endif

#if GS_CAN_NUM > 3
struct usbd_endpoint winusb_out_ep4 = {
    .ep_addr = GSUSB_ENDPOINT_OUT4,
    .ep_cb = usbd_winusb_out
};

struct usbd_endpoint winusb_in_ep4 = {
    .ep_addr = GSUSB_ENDPOINT_IN4,
    .ep_cb = usbd_winusb_in
};
#endif

struct usbd_interface intf0;
struct usbd_interface intf1;
struct usbd_interface intf2;
struct usbd_interface intf3;

void winusb_init(uint8_t busid, uint32_t reg_base)
{
    usbd_desc_register(busid, &winusb_descriptor);

    usbd_add_interface(busid, usbd_gs_can_init_intf(busid, &intf0));
    usbd_add_endpoint(busid, &winusb_out_ep1);
    usbd_add_endpoint(busid, &winusb_in_ep1);

#if GS_CAN_NUM > 1
    usbd_add_interface(busid, usbd_gs_can_init_intf(busid, &intf1));
    usbd_add_endpoint(busid, &winusb_out_ep2);
    usbd_add_endpoint(busid, &winusb_in_ep2);
#endif
#if GS_CAN_NUM > 2
    usbd_add_interface(busid, usbd_gs_can_init_intf(busid, &intf2));
    usbd_add_endpoint(busid, &winusb_out_ep3);
    usbd_add_endpoint(busid, &winusb_in_ep3);
#endif
#if GS_CAN_NUM > 3
    usbd_add_interface(busid, usbd_gs_can_init_intf(busid, &intf3));
    usbd_add_endpoint(busid, &winusb_out_ep4);
    usbd_add_endpoint(busid, &winusb_in_ep4);
#endif
    usbd_initialize(busid, reg_base, usbd_event_handler);
}
