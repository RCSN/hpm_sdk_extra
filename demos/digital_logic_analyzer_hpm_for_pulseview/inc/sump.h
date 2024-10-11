/*
 * sump.h
 *
 *  Created on: 11.11.2012
 *      Author: user
 */

#ifndef SUMP_H_
#define SUMP_H_

#include "hpm_common.h"
//frequency base of the original SUMP logic analyzer
//used for converting frequency dividers
#define SUMP_ORIGINAL_FREQ	(100000000)

//short sump commands
#define SUMP_CMD_RESET	0x00
#define SUMP_CMD_RUN	0x01
#define SUMP_CMD_ID		0x02
#define SUMP_CMD_META	0x04
//long sump commands
#define SUMP_CMD_SET_SAMPLE_RATE	0x80
#define SUMP_CMD_SET_COUNTS			0x81
#define SUMP_CMD_SET_FLAGS			0x82
//basic triggers
#define SUMP_CMD_SET_BT0_MASK		0xC0
#define SUMP_CMD_SET_BT0_VALUE		0xC1

#define SUMP_META_END	0x00
#define SUMP_META_NAME	0x01
#define SUMP_META_FPGA_VERSION	0x02
#define SUMP_META_CPU_VERSION	0x03
#define SUMP_META_PROBES_DW		0x20
#define SUMP_META_SAMPLE_RAM	0x21
#define SUMP_META_DYNAMIC_RAM	0x22
#define SUMP_META_SAMPLE_RATE	0x23
#define SUMP_META_PROTOCOL		0x24
#define SUMP_META_PROBES_B		0x40
#define SUMP_META_PROTOCOL_B	0x41

#define SUMP_FLAG1_DDR			0x0001
#define SUMP_FLAG1_NOISE_FILTER	0x0002
#define SUMP_FLAG1_GROUPS		0x003C
#define SUMP_FLAG1_GR0_DISABLE	0x0004
#define SUMP_FLAG1_GR1_DISABLE	0x0008
#define SUMP_FLAG1_GR2_DISABLE	0x0010
#define SUMP_FLAG1_GR3_DISABLE	0x0020
#define SUMP_FLAG1_ENABLE_RLE	0x0100

#define SUMP_FLAG1_GR_8BIT		(SUMP_FLAG1_GR1_DISABLE | SUMP_FLAG1_GR2_DISABLE | SUMP_FLAG1_GR3_DISABLE)
#define SUMP_FLAG1_GR_16BIT		(SUMP_FLAG1_GR2_DISABLE | SUMP_FLAG1_GR3_DISABLE)
#define SUMP_FLAG1_GR_32BIT		(0)

#define maxSampleRate 200000000
#define maxSampleMemory (20*1024)

#define BYTE1(v) ((uint8_t)v & 0xff)         //LSB
#define BYTE2(v) ((uint8_t)(v >> 8) & 0xff)  //
#define BYTE3(v) ((uint8_t)(v >> 16) & 0xff) //
#define BYTE4(v) ((uint8_t)(v >> 24) & 0xff) //MSB

int sump_process_request(uint8_t *buffer, uint16_t len);

#endif /* SUMP_H_ */
