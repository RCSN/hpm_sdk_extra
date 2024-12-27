/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_spi_drv.h"
#include "hpm_clock_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_gpiom_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_csr_drv.h"
#include "hpm_serial_nor.h"
#include "hpm_serial_nor_host_port.h"

#include "usb_config.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef PLACE_BUFF_AT_CACHEABLE
#define PLACE_BUFF_AT_CACHEABLE 1
#endif

extern void msc_spi_flash_init(uint8_t busid, uintptr_t reg_base);

hpm_serial_nor_t nor_flash_dev = {0};

int main(void)
{
    hpm_stat_t stat;
    hpm_serial_nor_info_t flash_info;
    board_init();
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    serial_nor_get_board_host(&nor_flash_dev.host);
    board_init_spi_clock(nor_flash_dev.host.host_param.param.host_base);
    serial_nor_spi_pins_init(nor_flash_dev.host.host_param.param.host_base);

    stat = hpm_serial_nor_init(&nor_flash_dev, &flash_info);
    if (stat != status_success) {
        printf("spi nor flash init error\n");
    } else {
        if (hpm_serial_nor_get_info(&nor_flash_dev, &flash_info) == status_success){
            printf("the flash size:%d KB\n", flash_info.size_in_kbytes);
            printf("the flash page_size:%d Byte\n", flash_info.page_size);
            printf("the flash sector_size:%d KB\n", flash_info.sector_size_kbytes);
            printf("the flash block_size:%d KB\n", flash_info.block_size_kbytes);
            printf("the flash sector_erase_cmd:0x%02x\n", flash_info.sector_erase_cmd);
            printf("the flash block_erase_cmd:0x%02x\n", flash_info.block_erase_cmd);
        }
        printf("spi nor flash init ok\n");
        msc_spi_flash_init(0, CONFIG_HPM_USBD_BASE);
        vTaskStartScheduler();
    }
    while (1) {
    };
}
