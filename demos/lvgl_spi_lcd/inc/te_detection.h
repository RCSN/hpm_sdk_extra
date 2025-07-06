/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
*/
#ifndef TE_DETECTION_H
#define TE_DETECTION_H
#include "hpm_common.h"
void te_detection_init(void);
bool get_te_detection_status(void);
void set_te_detection_status(bool status);
#endif