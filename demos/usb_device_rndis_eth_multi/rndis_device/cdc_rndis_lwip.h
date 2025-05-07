/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _CDC_RNDIS_LWIP_H_
#define _CDC_RNDIS_LWIP_H_
#include "hpm_common.h"
void cdc_rndis_lwip_task(void *pvParameters);
bool check_dhcp_success_status(void);
#endif