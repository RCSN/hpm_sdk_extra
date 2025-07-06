#ifndef HSLINK_PRO_USB2CAN_H
#define HSLINK_PRO_USB2CAN_H
#include "cdc_acm.h"

#define RX_BUFFER_SIZE                             2048U
#define TX_BUFFER_SIZE                             2048U

typedef struct cdc_device_cfg{
    struct usbd_endpoint cdc_out_ep;
    struct usbd_endpoint cdc_in_ep;
    struct usbd_interface intf0;
    struct usbd_interface intf1;
    struct cdc_line_coding s_line_coding;
    bool is_open;
    bool ep_tx_busy_flag;
} cdc_device_cfg_t;

typedef struct {
    cdc_device_cfg_t cdc_device;
    uint8_t read_buffer[RX_BUFFER_SIZE];
    uint8_t write_buffer[RX_BUFFER_SIZE];
    chry_ringbuffer_t usb_out_rb;
    uint8_t usb_out_mempool[1024];
} cdc_can_device_t;


void cdc_acm_can_init(void);
void usbd_init_line_coding(cdc_can_device_t *cdc_can_device, struct cdc_line_coding *line_coding);
uint8_t *usbd_get_write_buffer_ptr(cdc_can_device_t *cdc_can_device);
uint8_t *usbd_get_read_buffer_ptr(cdc_can_device_t *cdc_can_device);
bool usbd_is_tx_busy(cdc_can_device_t *cdc_can_device);
void usbd_set_tx_busy(cdc_can_device_t *cdc_can_device);
bool get_usb_out_char(cdc_can_device_t *cdc_can_device, uint8_t *data);
bool get_usb_out_is_empty(cdc_can_device_t *cdc_can_device);
uint32_t write_usb_data(cdc_can_device_t *cdc_can_device, uint8_t *data, uint32_t len);
#endif //HSLINK_PRO_USB2UART_H
