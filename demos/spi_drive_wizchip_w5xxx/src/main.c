/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_l1c_drv.h"
#include "hpm_mchtmr_drv.h"
#include "port.h"
#include "wizchip_conf.h"
#include  "socket.h"
#include  "dhcp.h"
#include "loopback.h"
#include _WIZCHIP_STR_

wiz_NetInfo g_winznet_info;

uint8_t ar[16] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};
uint8_t dhcp_buff[2048];
uint8_t tcpc_buff[2048];

static void load_net_parameters(void)
{
    g_winznet_info.gw[0] = 192;
    g_winznet_info.gw[1] = 168;
    g_winznet_info.gw[2] = 0;
    g_winznet_info.gw[3] = 1;

    g_winznet_info.sn[0] = 255;
    g_winznet_info.sn[1] = 255;
    g_winznet_info.sn[2] = 255;
    g_winznet_info.sn[3] = 0;

    g_winznet_info.mac[0] = 0x0c;
    g_winznet_info.mac[1] = 0x29;
    g_winznet_info.mac[2] = 0xab;
    g_winznet_info.mac[3] = 0x7c;
    g_winznet_info.mac[4] = 0x00;
    g_winznet_info.mac[5] = 0x01;

    g_winznet_info.ip[0] = 192;
    g_winznet_info.ip[1] = 168;
    g_winznet_info.ip[2] = 0;
    g_winznet_info.ip[3] = 246;

    g_winznet_info.dhcp = NETINFO_STATIC;
}

static void network_init(void)
{
    ctlnetwork(CN_SET_NETINFO, (void*)&g_winznet_info);
}
static void my_ip_assign(void)
{
   getIPfromDHCP(g_winznet_info.ip);
   getGWfromDHCP(g_winznet_info.gw);
   getSNfromDHCP(g_winznet_info.sn);
   getDNSfromDHCP(g_winznet_info.dns);
   g_winznet_info.dhcp = NETINFO_DHCP;
   /* Network initialization */
   network_init();      // apply from dhcp
   printf("DHCP LEASED TIME : %d Sec.\r\n", getDHCPLeasetime());
}

static void my_ip_conflict(void)
{
	printf("CONFLICT IP from DHCP\r\n");
	//halt or reset or any...
	while(1); // this example is halt.
}

int main(void)
{
    uint8_t tmp;
    uint8_t destip[4] = {192, 168, 0, 113};
    board_init();
    wizchip_spi_init();
    wizchip_register_port();
    load_net_parameters();
    if (ctlwizchip(CW_INIT_WIZCHIP, ar) == -1) {
        printf("WIZCHIP Initialized fail.\r\n");
        while(1);
    }
    do{
        if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1){
            board_delay_ms(10);
            printf("Unknown PHY Link stauts.\r\n");
        }
    }while(tmp == PHY_LINK_OFF);
    printf("SIP: %d.%d.%d.%d\r\n", g_winznet_info.ip[0],g_winznet_info.ip[1],g_winznet_info.ip[2],g_winznet_info.ip[3]);
    printf("GAR: %d.%d.%d.%d\r\n", g_winznet_info.gw[0],g_winznet_info.gw[1],g_winznet_info.gw[2],g_winznet_info.gw[3]);
    printf("SUB: %d.%d.%d.%d\r\n", g_winznet_info.sn[0],g_winznet_info.sn[1],g_winznet_info.sn[2],g_winznet_info.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", g_winznet_info.dns[0],g_winznet_info.dns[1],g_winznet_info.dns[2],g_winznet_info.dns[3]);

#if defined(CONFIG_WIZNET_DCHP) || (CONFIG_WIZNET_DCHP == 1)
    setSHAR(g_winznet_info.mac);
    DHCP_init(0,dhcp_buff);
    reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
    uint8_t dhcp_ret = DHCP_run();
    while(dhcp_ret != DHCP_IP_LEASED) {
        printf("hello world %d\n",dhcp_ret);
        board_delay_ms(1000);
        dhcp_ret = DHCP_run();
    }
    printf("dhcp okkkk\n");
#endif

    printf("SIP: %d.%d.%d.%d\r\n", g_winznet_info.ip[0],g_winznet_info.ip[1],g_winznet_info.ip[2],g_winznet_info.ip[3]);
    printf("GAR: %d.%d.%d.%d\r\n", g_winznet_info.gw[0],g_winznet_info.gw[1],g_winznet_info.gw[2],g_winznet_info.gw[3]);
    printf("SUB: %d.%d.%d.%d\r\n", g_winznet_info.sn[0],g_winznet_info.sn[1],g_winznet_info.sn[2],g_winznet_info.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", g_winznet_info.dns[0],g_winznet_info.dns[1],g_winznet_info.dns[2],g_winznet_info.dns[3]);
    memset(tcpc_buff, 0x66, sizeof(tcpc_buff));
    while (1) {
#if defined(CONFIG_TCP_CLIENT_IPERF) || (CONFIG_TCP_CLIENT_IPERF == 1)
        loopback_tcpc(1, tcpc_buff, destip, 5001);
#else
        board_delay_ms(1);
#endif
    }
}
