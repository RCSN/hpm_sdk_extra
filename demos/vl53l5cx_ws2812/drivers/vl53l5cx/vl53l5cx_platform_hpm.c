/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * VL53L5CX platform porting layer for HPM SDK
 * - Uses hpm_i2c component for I2C communication
 * - Supports dual sensors on separate I2C buses
 * - Extended VL53L5CX_Platform with user_data for I2C context
 * - All I2C/GPIO pins self-initialized, defined via CMakeLists.txt
 */

#include "vl53l5cx_platform.h"
#include "hpm_i2c.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"
#include "hpm_gpiom_soc_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_soc.h"
#include <board.h>
#include <string.h>

/*
 * Pin definitions from CMakeLists.txt sdk_compile_definitions.
 * I2C pads use ALT_SELECT_SET(4) for I2C function on HPM5300.
 * GPIO pads use ALT_SELECT_SET(0) for GPIO function.
 *
 *                   Sensor A    Sensor B
 *   I2C SCL        PB08         PB13
 *   I2C SDA        PB09         PB12
 *   PWR_EN         PA09         PA31
 *   LPn            PB10         PB14
 *   I2C_RST        PB11         PB15
 */

#define VL53L5CX_GPIO_CTRL    HPM_GPIO0
#define I2C_FUNC_ALT          4   /* I2C function alt-select on HPM5300 */
#define GPIO_FUNC_ALT         0   /* GPIO function alt-select */

/*
 * HPM I2C has a max transfer size per transaction (I2C_SOC_TRANSFER_COUNT_MAX = 4096).
 * The limit applies to addr_size + buf_size combined, so with 2-byte address,
 * the max data per chunk is 4096 - 2 = 4094 bytes.
 * VL53L5CX firmware download uses WrMulti with up to 0x8000 (32KB) at once.
 * We must split large transfers into chunks that fit the I2C HW limit.
 */
#define I2C_CHUNK_SIZE  (4095 - 2)  /* 12bit 0b111111111111 = 4095, addr_size=2  so max data = 4095-2 = 4093 */

/* SDK provides GPIO_GET_PORT_INDEX(pad) = pad/32 and GPIO_GET_PIN_INDEX(pad) = pad%32 */

/* Assign pad to GPIO0 controller and make it visible (similar to CJTAG_DP pattern) */
static void gpiom_assign_pin_to_gpio0(uint32_t pad)
{
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(pad), GPIO_GET_PIN_INDEX(pad), gpiom_soc_gpio0);
    gpiom_enable_pin_visibility(HPM_GPIOM, GPIO_GET_PORT_INDEX(pad), GPIO_GET_PIN_INDEX(pad), gpiom_soc_gpio0);
}

/* Per-sensor pin table */
typedef struct {
    uint32_t scl_pad;
    uint32_t sda_pad;
    uint32_t pwr_en_pad;
    uint32_t lpn_pad;
    uint32_t i2c_rst_pad;
} vl53l5cx_sensor_pins_t;

static const vl53l5cx_sensor_pins_t sensor_pins[2] = {
    { /* Sensor A */
        .scl_pad     = VL53L5CX_A_I2C_SCL,
        .sda_pad     = VL53L5CX_A_I2C_SDA,
        .pwr_en_pad  = VL53L5CX_A_PWR_EN,
        .lpn_pad     = VL53L5CX_A_LPN,
        .i2c_rst_pad = VL53L5CX_A_I2C_RST,
    },
    { /* Sensor B */
        .scl_pad     = VL53L5CX_B_I2C_SCL,
        .sda_pad     = VL53L5CX_B_I2C_SDA,
        .pwr_en_pad  = VL53L5CX_B_PWR_EN,
        .lpn_pad     = VL53L5CX_B_LPN,
        .i2c_rst_pad = VL53L5CX_B_I2C_RST,
    },
};

/* Initialize I2C pad: set ALT_SELECT(4) + pull-up + open-drain enable */
static void i2c_pad_init(uint32_t pad)
{
    HPM_IOC->PAD[pad].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(I2C_FUNC_ALT) | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
    HPM_IOC->PAD[pad].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1)  /* pull-up */
                                | IOC_PAD_PAD_CTL_OD_SET(1);  /* open-drain */
    /* I2C pads don't need GPIOM assignment — peripheral drives them directly */
}

/* Initialize GPIO pad as output, default LOW */
static void gpio_pad_init_out_low(uint32_t pad)
{
    HPM_IOC->PAD[pad].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_FUNC_ALT);
    HPM_IOC->PAD[pad].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0); /* pull-down */
    gpiom_assign_pin_to_gpio0(pad);
    gpio_set_pin_output(VL53L5CX_GPIO_CTRL, GPIO_GET_PORT_INDEX(pad), GPIO_GET_PIN_INDEX(pad));
    gpio_write_pin(VL53L5CX_GPIO_CTRL, GPIO_GET_PORT_INDEX(pad), GPIO_GET_PIN_INDEX(pad), 0);
}

/* Write GPIO pad */
static void gpio_pad_write(uint32_t pad, uint8_t val)
{
    gpio_write_pin(VL53L5CX_GPIO_CTRL, GPIO_GET_PORT_INDEX(pad), GPIO_GET_PIN_INDEX(pad), val);
}

/* I2C contexts for dual sensors */
static hpm_i2c_context_t i2c_ctx_sensor_a;
static hpm_i2c_context_t i2c_ctx_sensor_b;

/* Sensor A: I2C2 (PB08=SCL, PB09=SDA) */
/* Sensor B: I2C3 (PB13=SCL, PB12=SDA) = BOARD_APP_I2C */

/*
 * Initialize all VL53L5CX-SATEL control pins (PWR_EN, LPn, I2C_RST)
 * All pins default LOW: sensors unpowered, I2C disabled, not in reset.
 */
void vl53l5cx_hpm_gpio_init(void)
{
    for (int i = 0; i < 2; i++) {
        gpio_pad_init_out_low(sensor_pins[i].pwr_en_pad);
        gpio_pad_init_out_low(sensor_pins[i].lpn_pad);
        gpio_pad_init_out_low(sensor_pins[i].i2c_rst_pad);
    }
}

/*
 * Power-up and enable sequence for VL53L5CX-SATEL:
 *   1. Drive PWR_EN HIGH   -> power on the sensor (onboard regulator)
 *   2. Wait for power stable (regulator + sensor internal boot)
 *   3. Drive LPn HIGH      -> enable I2C communication
 *   4. Wait for sensor I2C ready
 *
 * Note: I2C_RST is NOT pulsed during normal power-up.
 *       It is only used for recovery via vl53l5cx_hpm_sensor_reset().
 *       Pulsing I2C_RST during boot can disrupt the sensor's internal
 *       initialization sequence.
 */
void vl53l5cx_hpm_sensor_enable(uint8_t sensor_id)
{
    const vl53l5cx_sensor_pins_t *pins = &sensor_pins[sensor_id];

    /* Step 1: Power on */
    gpio_pad_write(pins->pwr_en_pad, 1);
    board_delay_ms(10); /* Wait for onboard regulator + sensor POR */

    /* Step 2: Enable I2C comms */
    gpio_pad_write(pins->lpn_pad, 1);
    board_delay_ms(10); /* Wait for sensor internal ROM boot */
}

/*
 * Power-down sequence: disable I2C, then cut power.
 */
void vl53l5cx_hpm_sensor_disable(uint8_t sensor_id)
{
    const vl53l5cx_sensor_pins_t *pins = &sensor_pins[sensor_id];

    /* Disable I2C first, then cut power */
    gpio_pad_write(pins->lpn_pad, 0);
    board_delay_ms(1);
    gpio_pad_write(pins->pwr_en_pad, 0);
}

/*
 * I2C interface reset (active HIGH pulse).
 * Use for recovery when I2C communication is stuck,
 * NOT during normal power-up sequence.
 */
void vl53l5cx_hpm_sensor_reset(uint8_t sensor_id)
{
    const vl53l5cx_sensor_pins_t *pins = &sensor_pins[sensor_id];

    gpio_pad_write(pins->i2c_rst_pad, 1);
    board_delay_ms(5);
    gpio_pad_write(pins->i2c_rst_pad, 0);
    board_delay_ms(5);
}

void vl53l5cx_hpm_i2c_init(void)
{
    /* Sensor A: I2C2 — self-initialize pads, bypass board.c pinmux */
    hpm_i2c_get_default_init_context(&i2c_ctx_sensor_a);
    i2c_ctx_sensor_a.base = HPM_I2C2;
    i2c_ctx_sensor_a.init_config.communication_mode = i2c_master;
    i2c_ctx_sensor_a.init_config.speed = i2c_speed_1Mhz;
    i2c_ctx_sensor_a.init_config.is_10bit_addressing = false;
    i2c_ctx_sensor_a.init_config.slave_address = 0x29; /* VL53L5CX 7-bit addr */
    i2c_ctx_sensor_a.addr_endianness = i2c_master_addr_big_endian; /* VL53L5CX uses 16-bit reg addr, MSB first */
    clock_add_to_group(clock_i2c2, 0);
    i2c_pad_init(sensor_pins[0].scl_pad);
    i2c_pad_init(sensor_pins[0].sda_pad);
    hpm_i2c_initialize(&i2c_ctx_sensor_a);

    /* Sensor B: I2C3 — self-initialize pads, bypass board.c pinmux */
    hpm_i2c_get_default_init_context(&i2c_ctx_sensor_b);
    i2c_ctx_sensor_b.base = HPM_I2C3;
    i2c_ctx_sensor_b.init_config.communication_mode = i2c_master;
    i2c_ctx_sensor_b.init_config.speed = i2c_speed_1Mhz;
    i2c_ctx_sensor_b.init_config.is_10bit_addressing = false;
    i2c_ctx_sensor_b.init_config.slave_address = 0x29;
    i2c_ctx_sensor_b.addr_endianness = i2c_master_addr_big_endian; /* VL53L5CX uses 16-bit reg addr, MSB first */
    clock_add_to_group(clock_i2c3, 0);
    i2c_pad_init(sensor_pins[1].scl_pad);
    i2c_pad_init(sensor_pins[1].sda_pad);
    hpm_i2c_initialize(&i2c_ctx_sensor_b);
}


/*
 * Route I2C context via user_data field in VL53L5CX_Platform.
 * Both sensors have the same I2C address (0x52), so we can't route by address.
 * Instead, user_data stores the hpm_i2c_context_t pointer, which survives
 * struct copies (e.g., vl53l5cx_dev_a.platform = *get_sensor_a_platform()).
 */
static hpm_i2c_context_t *get_i2c_ctx(VL53L5CX_Platform *p_platform)
{
    return (hpm_i2c_context_t *)p_platform->user_data;
}

/* I2C write function for VL53L5CX */
static int32_t vl53l5cx_i2c_write(VL53L5CX_Platform *p_platform, uint16_t reg, uint8_t *data, uint16_t size)
{
    uint8_t addr7 = (p_platform->address >> 1) & 0x7F;
    hpm_i2c_context_t *ctx = get_i2c_ctx(p_platform);
    if (ctx == NULL) {
        return -1;
    }

    /* Split into I2C HW-compatible chunks */
    uint16_t offset = 0;
    while (offset < size) {
        uint16_t chunk = size - offset;
        if (chunk > I2C_CHUNK_SIZE) {
            chunk = I2C_CHUNK_SIZE;
        }
        hpm_stat_t stat = hpm_i2c_master_addr_write_blocking(
            ctx, addr7, reg + offset, 2, data + offset, chunk, 0xFFFFFFFF);
        if (stat != status_success) {
            return -1;
        }
        offset += chunk;
    }
    return 0;
}

/* I2C read function for VL53L5CX */
static int32_t vl53l5cx_i2c_read(VL53L5CX_Platform *p_platform, uint16_t reg, uint8_t *data, uint16_t size)
{
    uint8_t addr7 = (p_platform->address >> 1) & 0x7F;
    hpm_i2c_context_t *ctx = get_i2c_ctx(p_platform);
    if (ctx == NULL) {
        return -1;
    }

    /* Split into I2C HW-compatible chunks */
    uint16_t offset = 0;
    while (offset < size) {
        uint16_t chunk = size - offset;
        if (chunk > I2C_CHUNK_SIZE) {
            chunk = I2C_CHUNK_SIZE;
        }
        hpm_stat_t stat = hpm_i2c_master_addr_read_blocking(
            ctx, addr7, reg + offset, 2, data + offset, chunk, 0xFFFFFFFF);
        if (stat != status_success) {
            return -1;
        }
        offset += chunk;
    }
    return 0;
}

/* System tick for VL53L5CX */
static int32_t vl53l5cx_get_tick(void)
{
    /* Use mchtmr counter for ms timestamp */
    static uint32_t mchtmr_freq = 0;
    if (mchtmr_freq == 0) {
        mchtmr_freq = clock_get_frequency(clock_mchtmr0);
    }
    return (int32_t)(hpm_csr_get_core_cycle() / (mchtmr_freq / 1000));
}

/* Setup platform for sensor A */
VL53L5CX_Platform *vl53l5cx_hpm_get_sensor_a_platform(void)
{
    static VL53L5CX_Platform plat_a;
    plat_a.address = 0x52; /* VL53L5CX_DEFAULT_I2C_ADDRESS */
    plat_a.Write = vl53l5cx_i2c_write;
    plat_a.Read = vl53l5cx_i2c_read;
    plat_a.GetTick = vl53l5cx_get_tick;
    plat_a.user_data = &i2c_ctx_sensor_a;
    return &plat_a;
}

/* Setup platform for sensor B */
VL53L5CX_Platform *vl53l5cx_hpm_get_sensor_b_platform(void)
{
    static VL53L5CX_Platform plat_b;
    plat_b.address = 0x52; /* VL53L5CX_DEFAULT_I2C_ADDRESS */
    plat_b.Write = vl53l5cx_i2c_write;
    plat_b.Read = vl53l5cx_i2c_read;
    plat_b.GetTick = vl53l5cx_get_tick;
    plat_b.user_data = &i2c_ctx_sensor_b;
    return &plat_b;
}

/* ---- Mandatory platform functions for VL53L5CX ULD ---- */

uint8_t RdByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value)
{
    return (uint8_t)p_platform->Read(p_platform, RegisterAdress, p_value, 1U);
}

uint8_t WrByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value)
{
    return (uint8_t)p_platform->Write(p_platform, RegisterAdress, &value, 1U);
}

uint8_t RdMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values, uint32_t size)
{
    return (uint8_t)p_platform->Read(p_platform, RegisterAdress, p_values, size);
}

uint8_t WrMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values, uint32_t size)
{
    return (uint8_t)p_platform->Write(p_platform, RegisterAdress, p_values, size);
}

void SwapBuffer(uint8_t *buffer, uint16_t size)
{
    uint32_t i, tmp;
    for (i = 0; i < size; i = i + 4) {
        tmp = (buffer[i] << 24)
            | (buffer[i + 1] << 16)
            | (buffer[i + 2] << 8)
            | (buffer[i + 3]);
        memcpy(&(buffer[i]), &tmp, 4);
    }
}

uint8_t WaitMs(VL53L5CX_Platform *p_platform, uint32_t TimeMs)
{
    board_delay_ms(TimeMs);
    return 0;
}
