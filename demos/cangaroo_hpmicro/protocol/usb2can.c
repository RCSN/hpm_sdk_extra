#include "usb2can.h"
#include "slcan.h"


uint8_t *usbd_get_write_buffer_ptr(cdc_can_device_t *cdc_can_device)
{
    return (uint8_t *)cdc_can_device->write_buffer;
}

uint8_t *usbd_get_read_buffer_ptr(cdc_can_device_t *cdc_can_device)
{
    return (uint8_t *)cdc_can_device->read_buffer;
}

bool usbd_is_tx_busy(cdc_can_device_t *cdc_can_device)
{
    return cdc_can_device->cdc_device.ep_tx_busy_flag;
}

void usbd_set_tx_busy(cdc_can_device_t *cdc_can_device)
{
    cdc_can_device->cdc_device.ep_tx_busy_flag = true;
}

bool get_usb_out_char(cdc_can_device_t *cdc_can_device, uint8_t *data)
{
    return chry_ringbuffer_read_byte(&cdc_can_device->usb_out_rb, data);
}

bool get_usb_out_is_empty(cdc_can_device_t *cdc_can_device)
{
    return chry_ringbuffer_check_empty(&cdc_can_device->usb_out_rb);
}

uint32_t write_usb_data(cdc_can_device_t *cdc_can_device, uint8_t *data, uint32_t len)
{
    uint32_t res;

    if (usbd_is_tx_busy(cdc_can_device) == false) {
        usbd_set_tx_busy(cdc_can_device);
        res = usbd_ep_start_write(0, cdc_can_device->cdc_device.cdc_in_ep.ep_addr, data, len);
        if (res != 0) {
            return 0;
        }
    } else {
        return 0;
    }
    return len;
}

static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    uint16_t len = 0;
    uint8_t can_num = 0;
    cdc_can_device_t *cdc_can_device = NULL;
    //printf("ep:%d,can_num=%d\n", ep,i);
    //for (uint32_t j = 0; j < nbytes; j++) {
    //    printf("%c", g_cdc_can_device[i].read_buffer[j]);
    //}
#ifdef CONFIG_SLCAN0
    if(slcan0.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr == ep){
        cdc_can_device = &slcan0.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN1
    if(slcan1.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr == ep){
        cdc_can_device = &slcan1.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN2
    if(slcan2.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr == ep){
        cdc_can_device = &slcan2.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN3
    if(slcan3.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr == ep){
        cdc_can_device = &slcan3.g_cdc_can_device;
    }
#endif
    if(cdc_can_device != NULL){
        if ((cdc_can_device->read_buffer[0] == 'r') && (cdc_can_device->read_buffer[1] == '_') && (cdc_can_device->read_buffer[2] == 'c') &&
            (cdc_can_device->read_buffer[3] == 'a') && (cdc_can_device->read_buffer[4] == 'n')) {
                can_num = cdc_can_device->read_buffer[5] - '0';
                len = sprintf((char *)cdc_can_device->write_buffer, "HPM_CAN%d_BUS", can_num);
                usbd_ep_start_write(busid, 0x80 | ep, (const uint8_t *)cdc_can_device->write_buffer, len);
        } else {
            chry_ringbuffer_write(&cdc_can_device->usb_out_rb, cdc_can_device->read_buffer, nbytes);
        }
        usbd_ep_start_read(busid, ep, cdc_can_device->read_buffer, usbd_get_ep_mps(busid, ep));
    }
}

static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    cdc_can_device_t *cdc_can_device = NULL;
#ifdef CONFIG_SLCAN0
    if(slcan0.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr == ep){
        cdc_can_device = &slcan0.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN1
    if(slcan1.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr == ep){
        cdc_can_device = &slcan1.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN2
    if(slcan2.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr == ep){
        cdc_can_device = &slcan2.g_cdc_can_device;
    }
#endif
#ifdef CONFIG_SLCAN3
    if(slcan3.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr == ep){
        cdc_can_device = &slcan3.g_cdc_can_device;
    }
#endif
    if (((nbytes % usbd_get_ep_mps(busid, ep)) == 0) && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, ep, NULL, 0);
    } else {
        cdc_can_device->cdc_device.ep_tx_busy_flag = false;
    }
}

void cdc_acm_can_init(void)
{
    struct cdc_line_coding line_coding;
#ifdef CONFIG_SLCAN0
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan0.g_cdc_can_device.cdc_device.intf0));
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan0.g_cdc_can_device.cdc_device.intf1));
        slcan0.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr = CDC_OUT_EP;
        slcan0.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr = CDC_IN_EP;
        slcan0.g_cdc_can_device.cdc_device.cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        slcan0.g_cdc_can_device.cdc_device.cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(0, &slcan0.g_cdc_can_device.cdc_device.cdc_out_ep);
        usbd_add_endpoint(0, &slcan0.g_cdc_can_device.cdc_device.cdc_in_ep);
        slcan0.g_cdc_can_device.cdc_device.is_open = false;
        if (0 == chry_ringbuffer_init(&slcan0.g_cdc_can_device.usb_out_rb, slcan0.g_cdc_can_device.usb_out_mempool, sizeof(slcan0.g_cdc_can_device.usb_out_mempool))) {
            printf("slcan0 chry_ringbuffer_init success\r\n");
        } else {
            printf("slcan0 chry_ringbuffer_init error\r\n");
        }
        line_coding.dwDTERate = 1000000;
        line_coding.bDataBits = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = 0;
        usbd_init_line_coding(&slcan0.g_cdc_can_device, &line_coding);
#endif

#ifdef CONFIG_SLCAN1
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan1.g_cdc_can_device.cdc_device.intf0));
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan1.g_cdc_can_device.cdc_device.intf1));
        slcan1.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr = CDC_OUT_EP1;
        slcan1.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr = CDC_IN_EP1;
        slcan1.g_cdc_can_device.cdc_device.cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        slcan1.g_cdc_can_device.cdc_device.cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(0, &slcan1.g_cdc_can_device.cdc_device.cdc_out_ep);
        usbd_add_endpoint(0, &slcan1.g_cdc_can_device.cdc_device.cdc_in_ep);
        slcan1.g_cdc_can_device.cdc_device.is_open = false;
        if (0 == chry_ringbuffer_init(&slcan1.g_cdc_can_device.usb_out_rb, slcan1.g_cdc_can_device.usb_out_mempool, sizeof(slcan1.g_cdc_can_device.usb_out_mempool))) {
            printf("slcan1 chry_ringbuffer_init success\r\n");
        } else {
            printf("slcan1 chry_ringbuffer_init error\r\n");
        }
        line_coding.dwDTERate = 1000000;
        line_coding.bDataBits = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = 0;
        usbd_init_line_coding(&slcan1.g_cdc_can_device, &line_coding);
#endif

#ifdef CONFIG_SLCAN2
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan2.g_cdc_can_device.cdc_device.intf0));
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan2.g_cdc_can_device.cdc_device.intf1));
        slcan2.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr = CDC_OUT_EP2;
        slcan2.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr = CDC_IN_EP2;
        slcan2.g_cdc_can_device.cdc_device.cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        slcan2.g_cdc_can_device.cdc_device.cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(0, &slcan2.g_cdc_can_device.cdc_device.cdc_out_ep);
        usbd_add_endpoint(0, &slcan2.g_cdc_can_device.cdc_device.cdc_in_ep);
        slcan2.g_cdc_can_device.cdc_device.is_open = false;
        if (0 == chry_ringbuffer_init(&slcan2.g_cdc_can_device.usb_out_rb, slcan2.g_cdc_can_device.usb_out_mempool, sizeof(slcan2.g_cdc_can_device.usb_out_mempool))) {
            printf("slcan2 chry_ringbuffer_init success\r\n");
        } else {
            printf("slcan2 chry_ringbuffer_init error\r\n");
        }
        line_coding.dwDTERate = 1000000;
        line_coding.bDataBits = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = 0;
        usbd_init_line_coding(&slcan2.g_cdc_can_device, &line_coding);
#endif

#ifdef CONFIG_SLCAN3
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan3.g_cdc_can_device.cdc_device.intf0));
        usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &slcan3.g_cdc_can_device.cdc_device.intf1));
        slcan3.g_cdc_can_device.cdc_device.cdc_out_ep.ep_addr = CDC_OUT_EP3;
        slcan3.g_cdc_can_device.cdc_device.cdc_in_ep.ep_addr = CDC_IN_EP3;
        slcan3.g_cdc_can_device.cdc_device.cdc_out_ep.ep_cb = usbd_cdc_acm_bulk_out;
        slcan3.g_cdc_can_device.cdc_device.cdc_in_ep.ep_cb = usbd_cdc_acm_bulk_in;
        usbd_add_endpoint(0, &slcan3.g_cdc_can_device.cdc_device.cdc_out_ep);
        usbd_add_endpoint(0, &slcan3.g_cdc_can_device.cdc_device.cdc_in_ep);
        slcan3.g_cdc_can_device.cdc_device.is_open = false;
        if (0 == chry_ringbuffer_init(&slcan3.g_cdc_can_device.usb_out_rb, slcan3.g_cdc_can_device.usb_out_mempool, sizeof(slcan3.g_cdc_can_device.usb_out_mempool))) {
            printf("slcan3 chry_ringbuffer_init success\r\n");
        } else {
            printf("slcan3 chry_ringbuffer_init error\r\n");
        }
        line_coding.dwDTERate = 1000000;
        line_coding.bDataBits = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = 0;
        usbd_init_line_coding(&slcan3.g_cdc_can_device, &line_coding);
#endif
}

void usbd_init_line_coding(cdc_can_device_t *cdc_can_device, struct cdc_line_coding *line_coding)
{
    cdc_can_device->cdc_device.s_line_coding.dwDTERate = line_coding->dwDTERate;
    cdc_can_device->cdc_device.s_line_coding.bDataBits = line_coding->bDataBits;
    cdc_can_device->cdc_device.s_line_coding.bParityType = line_coding->bParityType;
    cdc_can_device->cdc_device.s_line_coding.bCharFormat = line_coding->bCharFormat;
}