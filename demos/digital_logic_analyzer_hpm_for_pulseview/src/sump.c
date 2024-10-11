/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "sump.h"
#include "usbd_core.h"
#include "cdc_acm.h"
#include "sampling.h"

const char metaData[] = {
    SUMP_META_NAME, 'h', 'p', 'm', 'i', 'c', 'r', 'o', '-', 'l', 'o','b', 's', '-', 'l', 'o', 'g', 'i', 'c', 0,
    SUMP_META_SAMPLE_RATE, BYTE4(maxSampleRate), BYTE3(maxSampleRate), BYTE2(maxSampleRate), BYTE1(maxSampleRate),
    SUMP_META_SAMPLE_RAM, 0, 0, BYTE2(maxSampleMemory), BYTE1(maxSampleMemory), //24*1024 b
    SUMP_META_PROBES_B, 32,
    SUMP_META_PROTOCOL_B, 1,
    SUMP_META_END
};

const uint8_t id[5] = "1ALS";

static uint32_t divider = 1;

uint32_t calc_local_divider(uint32_t divider, const uint32_t local_frequency, const uint32_t sump_frequency) 
{
    return local_frequency / (sump_frequency / (divider + 1)) - 1;
}

int sump_process_request(uint8_t *buffer, uint16_t len)
{
    int result = 0;
    printf(".....\r\n");
    switch(buffer[0])
    {
        case SUMP_CMD_RESET://reset
            lobs_close();
            printf("reset\r\n");
            result = 1;
            break;
        case SUMP_CMD_RUN://run
            printf("run\r\n");
            lobs_open();
            result = 1;
            break;
        case SUMP_CMD_ID://ID
            printf("id\r\n");
            if (!get_usb_cdc_tx_busy()) {
                set_usb_cdc_tx_busy(true);
                usbd_ep_start_write(0, CDC_IN_EP, (uint8_t*)id, 4);
            }
            result = 1;
            break;
        case SUMP_CMD_META://Query metas
            printf("meta\r\n");
            if (!get_usb_cdc_tx_busy()) {
                set_usb_cdc_tx_busy(true);
                usbd_ep_start_write(0, CDC_IN_EP, (uint8_t*)metaData, sizeof(metaData));
            }
            result = 1;
            break;
            case SUMP_CMD_SET_SAMPLE_RATE:
            if(len == 5)
            {
                //div120 = 120MHz / (100MHz / (div100 + 1)) - 1;
                printf("set sample rate\r\n");
                divider = *((uint32_t*)(buffer+1));
                //if maximum samplerate is 20MHz => 100/20 = 5, 5 - 1 = 4
                if(divider != 0)
                {
                    uint32_t clocks = 168000000; // сделать ф-ю для вычисления clocks
                    divider = calc_local_divider(divider, clocks, SUMP_ORIGINAL_FREQ);
                }
                if(divider == 0)
                {
                    divider = 1;
                }
                // SetSamplingPeriod(divider);
                result = 1;
            }
            break;
            case SUMP_CMD_SET_COUNTS:
            if(len == 5)
            {
                printf("set counts\r\n");
                uint16_t readCount  = 1 + *((uint16_t*)(buffer+1));
                uint16_t delayCount = *((uint16_t*)(buffer+3));
                // SetBufferSize(4 * readCount);
                // SetDelayCount(4 * delayCount);
                result = 1;
            }
            break;
        case SUMP_CMD_SET_BT0_MASK:
            if(len == 5)
            {
                printf("set bt0 mask\r\n");
                // SetTriggerMask(*(uint32_t*)(buffer+1));
                result = 1;
            }
            break;
        case SUMP_CMD_SET_BT0_VALUE:
            if(len == 5)
            {
                printf("set bt0 value\r\n");
                // SetTriggerValue(*(uint32_t*)(buffer+1));
                result = 1;
            }
            break;
        case SUMP_CMD_SET_FLAGS:
            if(len == 5)
            {
                printf("set flags\r\n");
                // SetFlags(*(uint16_t*)(buffer+1));
                result = 1;
                
            }
            break;
    }
    return result;
}
