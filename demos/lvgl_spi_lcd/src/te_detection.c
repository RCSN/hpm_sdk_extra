/*
 * Copyright (c) 2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
*/

#include "te_detection.h"

#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
#include <FreeRTOS.h>
#include <task.h>
#include "queue.h"
#include "semphr.h"
#endif

#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
SemaphoreHandle_t te_sem;
#else
static volatile bool te_come_flag = false;
#endif

void te_detection_init(void)
{
#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
    te_sem = xSemaphoreCreateCounting(10, 0);
    assert(te_sem != NULL);
#else
    te_come_flag = false;
#endif
}

bool get_te_detection_status(void)
{
#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
    if (xSemaphoreTake(te_sem, portMAX_DELAY) != pdTRUE) {
        return false;
    } else {
        return true;
    }
#else
    return te_come_flag;
#endif
}

void set_te_detection_status(bool status)
{
#if defined(USE_FREERTOS_OS) && (USE_FREERTOS_OS == 1)
    if (status == false) {
        return;
    }
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xPortIsInsideInterrupt()) {
        int ret = xSemaphoreGiveFromISR(te_sem, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        (void)xSemaphoreGive(te_sem);
    }
#else
    te_come_flag = status;
#endif
}