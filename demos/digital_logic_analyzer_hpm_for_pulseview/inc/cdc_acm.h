#ifndef CDC_ACM_H_
#define CDC_ACM_H_

#include "hpm_common.h"

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x01
#define CDC_INT_EP 0x83

bool get_usb_cdc_tx_busy(void);
void set_usb_cdc_tx_busy(bool busy);

#endif