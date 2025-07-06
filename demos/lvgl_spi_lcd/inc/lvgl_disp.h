/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef LVGL_DISP_H
#define LVGL_DISP_H
#include <stdint.h>
#include "hpm_common.h"

#ifndef LVGL_USE_TE_REFR
#define LVGL_USE_TE_REFR 0
#endif

#ifndef LVGL_USE_TE_SYNC
#define LVGL_USE_TE_SYNC 0
#endif

#ifndef LVGL_USE_FULL_BUFFER
#define LVGL_USE_FULL_BUFFER 0
#endif

#ifndef LVGL_USE_DIRECT_MODE
#define LVGL_USE_DIRECT_MODE 0
#endif

void lvgl_disp_init(uint8_t *disp_buf1, uint8_t *disp_buf2, uint32_t size_in_byte);
void lvgl_disp_te_handle(void);
#endif