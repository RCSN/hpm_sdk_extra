# FTP Server

## init wizchip
### variable and function

    uint8_t gFTPBUF[_MAX_SS];
    uint8_t wiznet_memsize[2][8] = {{8,8,8,8,8,8,8,8}, {8,8,8,8,8,8,8,8}};
    #define ETH_MAX_BUF_SIZE		2048
    uint8_t ethBuf0[ETH_MAX_BUF_SIZE];
    wiz_NetInfo gWIZNETINFO = {
            .mac = {0x00, 0x08, 0xdc, 0x11, 0x22, 0x33},
            .ip = {192, 168, 15, 111},
            .sn = {255, 255, 255, 0},
            .gw = {192, 168, 15, 1},
            .dns = {8, 8, 8, 8},
            .dhcp = NETINFO_STATIC



### run init funciton  in main funcion

    Reset_WIZCHIP();
	reg_wizchip_bus_cbfunc(Chip_read, Chip_write);
	reg_wizchip_cs_cbfunc(ChipCsEnable, ChipCsDisable);

	if (ctlwizchip(CW_INIT_WIZCHIP, (void*)wiznet_memsize) == -1)
	{
		printf("WIZchip memory initialization failed\r\n");
	}

	ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
	print_network_information();


## Run function

    if((ret = ftpd_run(gFTPBUF)) < 0)
	{
		porintf(" FTP server Error %d \r\n", ret);
	}

## FTP ID & PASSWORD

### USED ID
#### ID Enable in ftpd_init function

    ftp.ID_Enable = STATUS_USED;

#### ID Setting

    #define ftp_ID 		"wiznet"

### USED PASSWORD
#### PW Enable in ftpd_init function

    ftp.PW_Enable = STATUS_USED;

#### PW Setting

    #define ftp_PW 		"wiznet54321"

### Connedtion remain count

    #define remain_time 400000
