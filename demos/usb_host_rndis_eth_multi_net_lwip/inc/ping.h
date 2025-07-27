/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PING_H__
#define __PING_H__
#include <lwip/netif.h>
#include "hpm_common.h"

hpm_stat_t ping(struct netif *netif, char *target_name, uint32_t times, size_t size);

#endif
