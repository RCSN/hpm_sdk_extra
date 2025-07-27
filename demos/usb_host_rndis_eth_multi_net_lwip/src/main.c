/*
 * Copyright (c) 2024-2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*  HPM example includes. */
#include <stdio.h>
#include "hpm_uart_drv.h"
#include "common.h"
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "netconf.h"
#include "usbh_core.h"
#include "ping.h"

#ifndef CONFIG_PING_COUNT
#define CONFIG_PING_COUNT     (5U)
#endif

#if (USE_ENET_PORT_COUNT == 1)
#define NETIF_COUNT           (2U)
#elif (USE_ENET_PORT_COUNT == 2)
#define NETIF_COUNT           (3U)
#endif

/*--------------- Tasks Priority -------------*/
#define MAIN_TASK_PRIO   (tskIDLE_PRIORITY + 1)
#define DHCP_TASK_PRIO   (tskIDLE_PRIORITY + 4)
#define NETIF_STA_TASK_PRIO (tskIDLE_PRIORITY + 4)
#define PING_TASK_PRIO    (tskIDLE_PRIORITY + 5)

#if (USE_ENET_PORT_COUNT == 1)
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ENET_SOC_DESC_ADDR_ALIGNMENT)
__RW enet_rx_desc_t dma_rx_desc_tab[ENET_RX_BUFF_COUNT]; /* Ethernet Rx DMA Descriptor */

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ENET_SOC_DESC_ADDR_ALIGNMENT)
__RW enet_tx_desc_t dma_tx_desc_tab[ENET_TX_BUFF_COUNT]; /* Ethernet Tx DMA Descriptor */

ATTR_ALIGN(HPM_L1C_CACHELINE_SIZE)
__RW uint8_t rx_buff[ENET_RX_BUFF_COUNT][ENET_RX_BUFF_SIZE]; /* Ethernet Receive Buffer */

ATTR_ALIGN(HPM_L1C_CACHELINE_SIZE)
__RW uint8_t tx_buff[ENET_TX_BUFF_COUNT][ENET_TX_BUFF_SIZE]; /* Ethernet Transmit Buffer */
enet_desc_t desc;
uint8_t mac[ENET_MAC];
struct netif gnetif;
#elif (USE_ENET_PORT_COUNT == 2)
typedef struct {
    enet_rx_desc_t dma_rx_desc_tab[ENET_RX_BUFF_COUNT];
    enet_tx_desc_t dma_tx_desc_tab[ENET_TX_BUFF_COUNT];
    uint8_t        rx_buff[ENET_RX_BUFF_COUNT][ENET_RX_BUFF_SIZE];
    uint8_t        tx_buff[ENET_TX_BUFF_COUNT][ENET_TX_BUFF_SIZE];
} enet_desc_init_t;

ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(ENET_SOC_DESC_ADDR_ALIGNMENT)
enet_desc_init_t desc_init[BOARD_ENET_COUNT];
enet_desc_t desc[BOARD_ENET_COUNT];
uint8_t mac[BOARD_ENET_COUNT][ENET_MAC];
struct netif gnetif[BOARD_ENET_COUNT];
#endif

#if (NETIF_COUNT == 2)
char netif_name[NETIF_COUNT][4] = {
    "en0",
    "EX1",
};
#elif (NETIF_COUNT == 3)
char netif_name[NETIF_COUNT][4] = {
    "e00",
    "e11",
    "EX2",
};
#endif

void main_task(void *pvParameters);
static void ping_task(void *pdata);

void bsp_init(void)
{
    /* Initialize BSP */
    board_init();
#if (USE_ENET_PORT_COUNT == 1)
    /* Initialize GPIOs */
    board_init_enet_pins(ENET);
    /* Reset an enet PHY */
    board_reset_enet_phy(ENET);
#elif (USE_ENET_PORT_COUNT == 2)
    /* Initialize GPIOs */
    board_init_multiple_enet_pins();
    /* Reset Enet PHYs */
    board_reset_multiple_enet_phy();
#endif

    printf("This is an ethernet demo: TCP Echo With Socket API On FreeRTOS\n");
    printf("LwIP Version: %s\n", LWIP_VERSION_STRING);
#if (USE_ENET_PORT_COUNT == 1)
    #if defined(RGMII) && RGMII
    /* Set RGMII clock delay */
    board_init_enet_rgmii_clock_delay(ENET);
    #elif defined(RMII) && RMII
    /* Set RMII reference clock */
    board_init_enet_rmii_reference_clock(ENET, BOARD_ENET_RMII_INT_REF_CLK);
    printf("Reference Clock: %s\n", BOARD_ENET_RMII_INT_REF_CLK ? "Internal Clock" : "External Clock");
    #endif
#elif (USE_ENET_PORT_COUNT == 2)
   /* Initialize Enet Clock */
    board_init_multiple_enet_clock();
#endif
}

#if (USE_ENET_PORT_COUNT == 1)
hpm_stat_t enet_init(ENET_Type *ptr)
{
    enet_int_config_t int_config = {.int_enable = 0, .int_mask = 0};
    enet_mac_config_t enet_config;
    enet_tx_control_config_t enet_tx_control_config;

    #if defined(RGMII) && RGMII
        #if defined(__USE_DP83867) && __USE_DP83867
        dp83867_config_t phy_config;
        #else
        rtl8211_config_t phy_config;
        #endif
    #else
        #if defined(__USE_DP83848) && __USE_DP83848
        dp83848_config_t phy_config;
        #else
        rtl8201_config_t phy_config;
        #endif
    #endif

    /* Initialize td, rd and the corresponding buffers */
    memset((uint8_t *)dma_tx_desc_tab, 0x00, sizeof(dma_tx_desc_tab));
    memset((uint8_t *)dma_rx_desc_tab, 0x00, sizeof(dma_rx_desc_tab));
    memset((uint8_t *)rx_buff, 0x00, sizeof(rx_buff));
    memset((uint8_t *)tx_buff, 0x00, sizeof(tx_buff));

    desc.tx_desc_list_head = (enet_tx_desc_t *)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)dma_tx_desc_tab);
    desc.rx_desc_list_head = (enet_rx_desc_t *)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)dma_rx_desc_tab);

    desc.tx_buff_cfg.buffer = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)tx_buff);
    desc.tx_buff_cfg.count = ENET_TX_BUFF_COUNT;
    desc.tx_buff_cfg.size = ENET_TX_BUFF_SIZE;

    desc.rx_buff_cfg.buffer = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)rx_buff);
    desc.rx_buff_cfg.count = ENET_RX_BUFF_COUNT;
    desc.rx_buff_cfg.size = ENET_RX_BUFF_SIZE;

    /*Get a default control config for tx descriptor */
    enet_get_default_tx_control_config(ENET, &enet_tx_control_config);

    /* Set the control config for tx descriptor */
    memcpy(&desc.tx_control_config, &enet_tx_control_config, sizeof(enet_tx_control_config_t));

    /* Get MAC address */
    enet_get_mac_address(mac);

    /* Set MAC0 address */
    enet_config.mac_addr_high[0] = mac[5] << 8 | mac[4];
    enet_config.mac_addr_low[0]  = mac[3] << 24 | mac[2] << 16 | mac[1] << 8 | mac[0];
    enet_config.valid_max_count  = 1;

    /* Set DMA PBL */
    enet_config.dma_pbl = board_get_enet_dma_pbl(ENET);

    /* Set SARC */
    enet_config.sarc = enet_sarc_replace_mac0;

    /* Enable Enet IRQ */
    board_enable_enet_irq(ENET);

    /* Get the default interrupt config */
    enet_get_default_interrupt_config(ENET, &int_config);

    /* Initialize enet controller */
    if (enet_controller_init(ptr, ENET_INF_TYPE, &desc, &enet_config, &int_config) != status_success) {
        return status_fail;
    }

    /* Disable LPI interrupt */
    enet_disable_lpi_interrupt(ENET);

    /* Initialize phy */
    #if defined(RGMII) && RGMII
        #if defined(__USE_DP83867) && __USE_DP83867
        dp83867_reset(ptr);
        #if defined(__DISABLE_AUTO_NEGO) && __DISABLE_AUTO_NEGO
        dp83867_set_mdi_crossover_mode(ENET, enet_phy_mdi_crossover_manual_mdix);
        #endif
        dp83867_basic_mode_default_config(ptr, &phy_config);
        if (dp83867_basic_mode_init(ptr, &phy_config) == true) {
        #else
        rtl8211_reset(ptr);
        rtl8211_basic_mode_default_config(ptr, &phy_config);
        if (rtl8211_basic_mode_init(ptr, &phy_config) == true) {
        #endif
    #else
        #if defined(__USE_DP83848) && __USE_DP83848
        dp83848_reset(ptr);
        dp83848_basic_mode_default_config(ptr, &phy_config);
        if (dp83848_basic_mode_init(ptr, &phy_config) == true) {
        #else
        rtl8201_reset(ptr);
        rtl8201_basic_mode_default_config(ptr, &phy_config);
        if (rtl8201_basic_mode_init(ptr, &phy_config) == true) {
        #endif
    #endif
            printf("Enet phy init passed !\n");
            return status_success;
        } else {
            printf("Enet phy init failed !\n");
            return status_fail;
        }
}

#elif (USE_ENET_PORT_COUNT == 2)
static void enet_desc_init(enet_desc_t *pdesc, enet_desc_init_t *pdesc_init)
{
    pdesc->tx_desc_list_head = (enet_tx_desc_t *)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)pdesc_init->dma_tx_desc_tab);
    pdesc->rx_desc_list_head = (enet_rx_desc_t *)core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)pdesc_init->dma_rx_desc_tab);

    pdesc->tx_buff_cfg.buffer = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)pdesc_init->tx_buff);
    pdesc->tx_buff_cfg.count  = ENET_TX_BUFF_COUNT;
    pdesc->tx_buff_cfg.size   = ENET_TX_BUFF_SIZE;

    pdesc->rx_buff_cfg.buffer = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)pdesc_init->rx_buff);
    pdesc->rx_buff_cfg.count  = ENET_RX_BUFF_COUNT;
    pdesc->rx_buff_cfg.size   = ENET_RX_BUFF_SIZE;
}

static hpm_stat_t enet_init(uint8_t idx)
{
    enet_mac_config_t        enet_config;
    enet_tx_control_config_t enet_tx_control_config;
    enet_int_config_t        int_config = {0};
    enet_base_t              *base;
    enet_inf_type_t          itf;

    if (idx > BOARD_ENET_COUNT) {
        return status_invalid_argument;
    }

    base = board_get_enet_base(idx);
    itf  = board_get_enet_phy_itf(idx);

    /* Initialize td, rd and the corresponding buffers */
    enet_desc_init(&desc[idx], &desc_init[idx]);

    /* Get a default control config for tx descriptor */
    enet_get_default_tx_control_config(base, &enet_tx_control_config);

    /* Set the control config for tx descriptor */
    memcpy(&desc[idx].tx_control_config, &enet_tx_control_config, sizeof(enet_tx_control_config_t));

    /* Get a default MAC address */
    enet_get_mac_address(idx, mac[idx]);

    /* Set MAC0 address */
    enet_set_mac_address(&enet_config, mac[idx]);

    /* Set DMA PBL */
    enet_config.dma_pbl = board_get_enet_dma_pbl(base);

    /* Set SARC */
    enet_config.sarc = enet_sarc_replace_mac0;

    #if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
    /* Enable Enet IRQ */
    board_enable_enet_irq(base);

    /* Get the default interrupt config */
    enet_get_default_interrupt_config(ENET, &int_config);
    #endif

    /* Initialize enet controller */
    if (enet_controller_init(base, itf, &desc[idx], &enet_config, &int_config) != status_success) {
        printf("Enet%d MAC init failed!\n", idx);
        return status_fail;
    }

    /* Initialize Enet PHY */
    if (board_init_enet_phy(base) != status_success) {
        printf("Enet%d PHY init failed!\n", idx);
        return status_fail;
    }

    #if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
    /* Disable LPI interrupt */
    enet_disable_lpi_interrupt(base);
    #endif

    return status_success;
}
#endif

int main(void)
{
    /* Initialize bsp */
    bsp_init();
    board_init_usb((USB_Type *)CONFIG_HPM_USBH_BASE);
    /* set irq priority */
    intc_set_irq_priority(CONFIG_HPM_USBH_IRQn, 1);

    xTaskCreate(main_task, "Main", configMINIMAL_STACK_SIZE * 2, NULL, MAIN_TASK_PRIO, NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for ( ;; ) {

    }
}

void main_task(void *pvParameters)
{
    (void)pvParameters;

    TimerHandle_t timer_handle;
#if (USE_ENET_PORT_COUNT == 1)
    /* Initialize MAC and DMA */
    enet_init(ENET);

    /* Initialize LwIP stack */
    tcpip_init(NULL, NULL);
    netif_config(&gnetif);
#elif (USE_ENET_PORT_COUNT == 2)
    TaskFunction_t pxTaskCode[] = {netif0_update_link_status, netif1_update_link_status};
    char task_name[30] = {0};

    /* Initialize MAC and DMA */
    for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
        if (enet_init(i) == status_success) {
            printf("Enet%d init passed!\n", i);
        } else {
            printf("Enet%d init failed!\n", i);
        }
    }
    /* Initialize LwIP stack */
    tcpip_init(NULL, NULL);

    /* Initialize network setting, services and apps */
    for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
        netif_config(&gnetif[i], i);
    }
#endif
    usbh_initialize(0, CONFIG_HPM_USBH_BASE);

    xTaskCreate(ping_task, "ping_task", 2048, &gnetif, DEFAULT_THREAD_PRIO, NULL);

#if (USE_ENET_PORT_COUNT == 1)

#if defined(LWIP_DHCP) && LWIP_DHCP
    /* Start DHCP Client */
    xTaskCreate(LwIP_DHCP_task, "DHCP", configMINIMAL_STACK_SIZE * 2, &gnetif, DHCP_TASK_PRIO, NULL);
#endif

    xTaskCreate(netif_update_link_status, "netif update status", configMINIMAL_STACK_SIZE * 2, &gnetif, NETIF_STA_TASK_PRIO, NULL);

#elif (USE_ENET_PORT_COUNT == 2)
#if defined(LWIP_DHCP) && LWIP_DHCP
    /* Start DHCP Client */
    xTaskCreate(LwIP_DHCP_task, "DHCP", configMINIMAL_STACK_SIZE * 2, gnetif, DHCP_TASK_PRIO, NULL);
#endif

    for (uint8_t i = 0; i < BOARD_ENET_COUNT; i++) {
        sprintf(task_name, "Netif%d Link Status Update", i);
        xTaskCreate(pxTaskCode[i], task_name, configMINIMAL_STACK_SIZE * 2, &gnetif[i], NETIF_STA_TASK_PRIO, NULL);
    }

#endif

    timer_handle = xTimerCreate((const char *)NULL,
                                (TickType_t)1000,
                                (UBaseType_t)pdTRUE,
                                (void * const)1,
                                (TimerCallbackFunction_t)timer_callback);
    if (NULL != timer_handle)  {
        xTimerStart(timer_handle, 0);
    }

    vTaskDelete(NULL);
}



static void ping_task(void *pdata)
{
    (void)pdata;

    struct netif *netif[NETIF_COUNT] = {NULL};
    bool netif_up = true;
    bool netif_select = false;
    bool dhcp_check = false;
    bool ready_ping = false;
    uint8_t netif_index = 0;
    uint8_t dhcp_last_state[NETIF_COUNT];
    uint16_t i = 0;
    char buffer[50];
    char ch;
    struct dhcp *dhcp;
    while (1) {
        netif_up = true;
        for (i = 0; i < NETIF_COUNT; i++) {
            netif[i] = NULL;
        }
        for (i = 0; i < NETIF_COUNT; i++) {
            netif[i] = netif_find(netif_name[i]);
            vTaskDelay(100);
            if (netif[i] == NULL) {
                netif_up = false;
            } else if ((netif_is_up(netif[i]) == false) && (netif_dhcp_data(netif[i])->state != DHCP_STATE_BOUND)) {
                netif_up = false; 
            }
        }
        if (netif_up == false) {
            continue;
        }
        break;
    }

    while (1) {
#if LWIP_DHCP
        i = 0;
        netif_select = false;
        printf("\n*********************************************************************************\n");
        printf("\r\nPlease select which network card to use. Enter the corresponding number\r\n\r\n");
        for (i = 0; i < NETIF_COUNT; i++) {
            printf("%d: %c%c%d\r\n", i, netif[i]->name[0], netif[i]->name[1], netif[i]->num);
        }
        printf("*********************************************************************************\n");
        while (1) {
            if (uart_check_status(BOARD_CONSOLE_UART_BASE, uart_stat_data_ready) == true) {
                ch = uart_read_byte(BOARD_CONSOLE_UART_BASE);
                if (ch < '0' || ch > '9') {
                    break;
                }
                ch -= '0';
                if ((ch >= 0) && (ch < NETIF_COUNT)) {
                    netif_select = true;
                    break;
                }
            }
            vTaskDelay(100);
        }
        if (netif_select == false) {
            continue;
        }
        netif_index = ch;
        ready_ping = false;
        i = 0;
        dhcp_last_state[netif_index] = DHCP_STATE_OFF;
        if (netif_is_up(netif[netif_index])) {
            dhcp = netif_dhcp_data(netif[netif_index]);
            if (dhcp == NULL) {
                dhcp_last_state[netif_index] = DHCP_STATE_OFF;
            } else if (dhcp_last_state[netif_index] != dhcp->state) {
                dhcp_last_state[netif_index] = dhcp->state;
                if (dhcp_last_state[netif_index] == DHCP_STATE_BOUND) {
                    dhcp_check = true;
                } else {
                    dhcp_check = false;
                }
            }
            if (dhcp_check == true) {
                vTaskDelay(5);
                printf("\n***********************************************************"
                        "**********************\n");
                printf("\r\ninput ping the IP or URL address and "
                        "press the enter key to end\r\n\r\n");
                printf("if want to terminate midway, please press the esc key\r\n\r\n");
                printf("***********************************************************"
                        "**********************\n");
                while (1) {
                    if (uart_check_status(BOARD_CONSOLE_UART_BASE, uart_stat_data_ready) == true) {
                        ch = uart_read_byte(BOARD_CONSOLE_UART_BASE);
                        buffer[i++] = ch;
                        /*esc key*/
                        if (ch == 0x1B) {
                            break;
                        } else {
                            uart_send_byte(BOARD_CONSOLE_UART_BASE, ch);
                        }
                        /* enter key*/
                        if (ch == 0x0d) {
                            if (i > 5) {
                                ready_ping = true;
                            }
                            break;
                        }
                    }
                    if (netif_is_up(netif[netif_index]) == false) {
                        dhcp_check = false;
                        printf("%s is down\n", netif_name[netif_index]);
                        break;
                    }
                    vTaskDelay(10);
                }
                if (ready_ping) {
                    buffer[i - 1] = 0;
                    printf("netif: %c%c%d, ip: %s\n", netif[netif_index]->name[0],netif[netif_index]->name[1],
                            netif[netif_index]->num, ipaddr_ntoa(&netif[netif_index]->ip_addr));
                    ping(netif[netif_index], buffer, CONFIG_PING_COUNT, 0);
                }
            }
        } else {
            printf("%s is down\n", netif_name[netif_index]);
        }
#endif
        vTaskDelay(100);
    }
}


