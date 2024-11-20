#ifndef _SLCAN_H
#define _SLCAN_H

#include "stdint.h"
#include "stdio.h"
#include "string.h"

#include "hpm_mcan_drv.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Define
 */
/*---------------------------------------------------------------------------------------------------------*/

#ifndef SLCAN_DEBUG
#define SLCAN_DEBUG(...)
#endif

#define SCLAN_NUM 4


/* maximum rx buffer len: extended CAN frame with timestamp */
// #define SLCAN_MTU (sizeof("T1111222281122334455667788EA5F\r")+1)  //CAN2.0
#define SLCAN_MTU (sizeof("B12345678FEA5F\r") + 1 + 64 * 2) // CANFD

/*
If the Time Stamp is ON,
the incomming frames looks like this.t100211334D67[CR](a standard frame with
1D=0x100 & 2 bytes)Note the last 4 bytes 0x4D67, which is a Time Stamp for
thisspecific message in milliseconds (and of course in hex). The timerin the
CAN232 starts at zero 0x0000 and goes up to 0xEA5F beforeit loop arround and
get's back to 0x0000. This corresponds to exact60,000mS (i.e.1 minute which will
be more than enough in mostsystems).
*/

/* RUN FLAG */
#define SLCAN_RFLAG_CAN_OPEN 0x0001
#define SLCAN_RFLAG_CHAR_OPEN 0x0002
#define SLCAN_RFLAG_EXIT 0x8000

/* SET FLAG */
#define SLCAN_SFLAG_MODE 0x0001
#define SLCAN_SFLAG_BUADRATE 0x0002
#define SLCAN_SFLAG_TIMESTAMP 0x0004
#define SLCAN_SFLAG_LOG 0x0008

/* UART Config Parameters */
typedef struct {
    uint32_t u32BitRate;  /* Baud rate    */
    uint8_t u8CharFormat; /* stop bit     */
    uint8_t u8ParityType; /* parity       */
    uint8_t u8DataBits;   /* data bits    */
} UART_CFG_T;


struct slcan_t {
    MCAN_Type *ptr;
    uint32_t irq_num;
    uint8_t candev_sn;        // CAN operating channel
    mcan_config_t candev_cfg; // CAN configuration information
    int candev_oflag;       // candev open flag

    uint8_t uartdev_sn;     // uart operating channel
    UART_CFG_T uartdev_cfg; // uart configuration information
    int uartdev_oflag;      // uartdev open flag

    uint16_t run_state;
    uint16_t run_flag;
    uint16_t set_flag;
    uint8_t timestamp_isopen;
    uint8_t candev_isopen;

    mcan_tx_frame_t   can_tx_msg; // can TX message
    mcan_rx_message_t can_rx_msg; // can RX message

    uint8_t uart_rx_len;
    uint8_t uart_tx_len;
    uint8_t uart_rx_buffer[SLCAN_MTU]; // uart RX message
    uint8_t uart_tx_buffer[SLCAN_MTU]; // uart TX message
    uint32_t timestamp;
};

enum slcan_uart_status {
    SLCAN_UART_EXIT = 0,
    SLCAN_UART_ACK,
    SLCAN_UART_NACK,
    SLCAN_UART_REPLY,
    SLCAN_UART_SEND,
};

enum slcan_baud {
    SLCAN_BAUD_10K = 0,
    SLCAN_BAUD_20K,
    SLCAN_BAUD_50K,
    SLCAN_BAUD_100K,
    SLCAN_BAUD_125K,
    SLCAN_BAUD_250K,
    SLCAN_BAUD_500K,
    SLCAN_BAUD_800K,
    SLCAN_BAUD_1000K,
    SLCAN_BAUD_INVALID,
};

enum slcan_mode {
    SLCAN_MODE_NORMAL = 0,
    SLCAN_MODE_LISTEN,
    SLCAN_MODE_LOOPBACK,
    SLCAN_MODE_LOOPBACKANLISTEN,
};

/*---------------------------------------------------------------------------------------------------------*/
/*  Fuctions
 */
/*---------------------------------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
uint32_t sclcan_can_baud(uint8_t can_baud_index);

void slcan_can_open(struct slcan_t *slcan_port);
void slcan_can_close(struct slcan_t *slcan_port);
void slcan_can_set_nbaud(struct slcan_t *slcan_port,
                         uint8_t baud_index); // set normal baud rate
void slcan_can_set_dbaud(struct slcan_t *slcan_port,
                         uint8_t data_mbps); // set data baud rate

// read one message in sclan from canfd buffer
uint32_t slcan_can_read(struct slcan_t *slcan_port, uint8_t fifo_num);
// write one message to TX fifo  from canfd buffer  & send out
uint32_t slcan_can_write(struct slcan_t *slcan_port);

// read one message in sclan from uart
// return -1 : no data
// return 0 	: only 1 valid data
// return 1 	: There's still data left to process
int32_t slcan_uart_read(struct slcan_t *slcan_port);
// write one message to uart
uint32_t slcan_uart_write(struct slcan_t *slcan_port, void *buffer,
                          uint32_t size);

// Process receive and transmit message
void slcan_process_uart(struct slcan_t *slcan_port);
void slcan_process_can(struct slcan_t *slcan_port);
// uart & can process both in
void slcan_process_task(struct slcan_t *slcan_port);

// slcan instance initialization
void slcan_init(void);
void slcan_port0_init(void);
void slcan_port1_init(void);
void slcan_port2_init(void);
void slcan_port3_init(void);
void slcan_timestamp(void);

#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------------------------*/
/* Global variables */
/*------------------------------------------------------------------------------------------------*/


#endif // _SLCAN_H
