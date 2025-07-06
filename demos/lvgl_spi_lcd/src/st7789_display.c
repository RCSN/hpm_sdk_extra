/*
* Copyright (c) 2025 HPMicro
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/
 #if defined(ENABLE_ST7789_LCD_DRIVER) && (ENABLE_ST7789_LCD_DRIVER == 1)

#include "st7789_display.h"
#include "hpm_spi.h"
#include "hpm_spi_drv.h"
#include "board.h"
#include "lcd_font.h"

static spi_tft_lcd_context_t *st7789_lcd_ctx;

static void st7789_display_wr_reg(uint8_t dat)
{
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_set_dc(st7789_lcd_ctx, false);
    spi_tft_lcd_transfer_data_blocking(st7789_lcd_ctx, 8, (uint8_t *)&dat, 1, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
    spi_tft_lcd_set_dc(st7789_lcd_ctx, true);
}

static void st7789_display_wr_data8(uint8_t dat)
{
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(st7789_lcd_ctx, 8, (uint8_t *)&dat, 1, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
}

static void st7789_display_wr_data16(uint16_t dat)
{
#if 0
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(&st7789_lcd_ctx, 16, (uint8_t *)&dat, 2, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
#else
    st7789_display_wr_data8(dat >> 8);
    st7789_display_wr_data8(dat);
#endif
}

static void st7789_display_write_ram_blocking(uint8_t bit_width, uint8_t *data, uint32_t size, uint32_t timeout)
{
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(st7789_lcd_ctx, bit_width, data, size, timeout);
}

static void st7789_display_write_ram_nonblocking(uint8_t bit_width, uint8_t *data, uint32_t size)
{
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_nonblocking(st7789_lcd_ctx, bit_width, data, size);
}

static void st7789_display_cmd_init(void)
{
    if (st7789_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_set_rst(st7789_lcd_ctx, true);
    board_delay_ms(100);
    spi_tft_lcd_set_rst(st7789_lcd_ctx, false);
    board_delay_ms(100);
    spi_tft_lcd_set_rst(st7789_lcd_ctx, true);
    board_delay_ms(120);

//************* Start Initial Sequence **********//
    st7789_display_wr_reg(0x11); //Sleep out 
    board_delay_ms(120);           //Delay 120ms 
    //************* Start Initial Sequence **********// 
    st7789_display_wr_reg(0x36);
    if(USE_HORIZONTIAL == 0) {
        st7789_display_wr_data8(0x00);
    } else if (USE_HORIZONTIAL == 1) {
        st7789_display_wr_data8(0xC0);
    } else if (USE_HORIZONTIAL == 2) {
        st7789_display_wr_data8(0x70);
    } else {
        st7789_display_wr_data8(0xA0);
    }

    st7789_display_wr_reg(0x3A);
    st7789_display_wr_data8(0x05);

    st7789_display_wr_reg(0xB2);
    st7789_display_wr_data8(0x0C);
    st7789_display_wr_data8(0x0C);
    st7789_display_wr_data8(0x00);
    st7789_display_wr_data8(0x33);
    st7789_display_wr_data8(0x33);

    /* tearing effect line on*/
    st7789_display_wr_reg(0x35);
    st7789_display_wr_data8(0x00);

    st7789_display_wr_reg(0xB7);
    st7789_display_wr_data8(0x35);

    st7789_display_wr_reg(0xBB);
    st7789_display_wr_data8(0x32); //Vcom=1.35V

    st7789_display_wr_reg(0xC2);
    st7789_display_wr_data8(0x01);

    st7789_display_wr_reg(0xC3);
    st7789_display_wr_data8(0x15); //GVDD=4.8V  颜色深度

    st7789_display_wr_reg(0xC4);
    st7789_display_wr_data8(0x20); //VDV, 0x20:0v

    st7789_display_wr_reg(0xC6);
    st7789_display_wr_data8(0x1E); //0x1E:40Hz

    st7789_display_wr_reg(0xD0);
    st7789_display_wr_data8(0xA4);
    st7789_display_wr_data8(0xA1); 

    st7789_display_wr_reg(0xE0);
    st7789_display_wr_data8(0xD0);
    st7789_display_wr_data8(0x08);
    st7789_display_wr_data8(0x0E);
    st7789_display_wr_data8(0x09);
    st7789_display_wr_data8(0x09);
    st7789_display_wr_data8(0x05);
    st7789_display_wr_data8(0x31);
    st7789_display_wr_data8(0x33);
    st7789_display_wr_data8(0x48);
    st7789_display_wr_data8(0x17);
    st7789_display_wr_data8(0x14);
    st7789_display_wr_data8(0x15);
    st7789_display_wr_data8(0x31);
    st7789_display_wr_data8(0x34);

    st7789_display_wr_reg(0xE1);  
    st7789_display_wr_data8(0xD0);
    st7789_display_wr_data8(0x08);
    st7789_display_wr_data8(0x0E);
    st7789_display_wr_data8(0x09);
    st7789_display_wr_data8(0x09);
    st7789_display_wr_data8(0x15);
    st7789_display_wr_data8(0x31);
    st7789_display_wr_data8(0x33);
    st7789_display_wr_data8(0x48);
    st7789_display_wr_data8(0x17);
    st7789_display_wr_data8(0x14);
    st7789_display_wr_data8(0x15);
    st7789_display_wr_data8(0x31);
    st7789_display_wr_data8(0x34);
    st7789_display_wr_reg(0x21); 

    st7789_display_wr_reg(0x29);
}


void st7789_display_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if USE_HORIZONTIAL==0
    st7789_display_wr_reg(0x2a); /* 列地址设置 */
    st7789_display_wr_data16(x1 + ADDR_OFFSET);
    st7789_display_wr_data16(x2 + ADDR_OFFSET);
    st7789_display_wr_reg(0x2b); /* 行地址设置 */
    st7789_display_wr_data16(y1);
    st7789_display_wr_data16(y2);
    st7789_display_wr_reg(0x2c); /* 储存器写 */
#elif USE_HORIZONTIAL==1
    st7789_display_wr_reg(0x2a); /* 列地址设置 */
    st7789_display_wr_data16(x1 + ADDR_OFFSET);
    st7789_display_wr_data16(x2 + ADDR_OFFSET);
    st7789_display_wr_reg(0x2b); /* 行地址设置 */
    st7789_display_wr_data16(y1);
    st7789_display_wr_data16(y2);
    st7789_display_wr_reg(0x2c); /* 储存器写 */
#elif USE_HORIZONTIAL==2
    st7789_display_wr_reg(0x2a); /* 列地址设置 */
    st7789_display_wr_data16(x1);
    st7789_display_wr_data16(x2);
    st7789_display_wr_reg(0x2b); /* 行地址设置 */
    st7789_display_wr_data16(y1 + ADDR_OFFSET);
    st7789_display_wr_data16(y2 + ADDR_OFFSET);
    st7789_display_wr_reg(0x2c); /* 储存器写 */
#else
    st7789_display_wr_reg(0x2a); /* 列地址设置 */
    st7789_display_wr_data16(x1);
    st7789_display_wr_data16(x2);
    st7789_display_wr_reg(0x2b); /* 行地址设置 */
    st7789_display_wr_data16(y1 + ADDR_OFFSET);
    st7789_display_wr_data16(y2 + ADDR_OFFSET);
    st7789_display_wr_reg(0x2c); /* 储存器写 */
#endif
}


void st7789_display_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t i, j;
    st7789_display_address_set(x1, y1, x2 - 1, y2 - 1);
    for (j = y1; j < y2; j++) {
        for (i = x1; i < x2; i++) {
            st7789_display_wr_data16(color);
        }
    }
}

void st7789_display_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    st7789_display_address_set(x, y, x, y);
    st7789_display_wr_data16(color);
}

void st7789_display_show_char(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum; // 一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' ';              // 得到偏移后的值
    st7789_display_address_set(x, y, x + sizex - 1, y + sizey - 1); // 设置显示窗口
    for (i = 0; i < TypefaceNum; i++)
    {
        if (sizey == 12)
            temp = ascii_1206[num][i]; // 调用6x12字体
        else if (sizey == 16)
            temp = ascii_1608[num][i]; // 调用8x16字体
        else if (sizey == 24)
            temp = ascii_2412[num][i]; // 调用12x24字体
        else
            return;
        for (t = 0; t < 8; t++)
        {
            if (!mode) // 非叠加模式
            {
                if (temp & (0x01 << t)) {
                    st7789_display_wr_data16(fc);
                }
                else {
                    st7789_display_wr_data16(bc);
                }
                m++;
                if (m % sizex == 0)
                {
                    m = 0;
                    break;
                }
            }
            else // 叠加模式
            {
                if (temp & (0x01 << t))
                    st7789_display_draw_point(x, y, fc); // 画一个点
                x++;
                if ((x - x0) == sizex)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

void st7789_display_show_string(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint16_t sizey, uint8_t mode)
{
    while ((*s <= '~') && (*s >= ' ')) // 判断是不是非法字符
    {
        if (x > (ST7789_LCD_H_RES - 1) || y > (ST7789_LCD_V_RES - 1))
            return;
        st7789_display_show_char(x, y, *s, fc, bc, sizey, mode);
        x += sizey / 2;
        s++;
    }
}

void st7789_display_init(spi_tft_lcd_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    st7789_lcd_ctx = ctx;
    ctx->address_set = st7789_display_address_set;
    ctx->write_ram_blocking = st7789_display_write_ram_blocking;
    ctx->write_ram_nonblocking = st7789_display_write_ram_nonblocking;
    ctx->fill_color = st7789_display_fill_color;
    ctx->draw_point = st7789_display_draw_point;
    ctx->width = ST7789_LCD_H_RES;
    ctx->height = ST7789_LCD_V_RES;
    ctx->pixel_in_byte = 2;

    spi_tft_lcd_hardware_init(ctx);
    st7789_display_cmd_init();
    // st7789_display_fill_color(0, 0, ST7789_LCD_H_RES, ST7789_LCD_V_RES, 0xFFFF);
    // board_delay_ms(100);
    st7789_display_fill_color(0, 0, ST7789_LCD_H_RES, ST7789_LCD_V_RES, 0xFF00);
    board_delay_ms(100);
    // st7789_display_fill_color(0, 0, ST7789_LCD_H_RES, ST7789_LCD_V_RES, 0x00FF);
    // board_delay_ms(100);
    st7789_display_fill_color(0, 0, ST7789_LCD_H_RES, ST7789_LCD_V_RES, 0x0000);
    board_delay_ms(100);
    st7789_display_show_string(0, 0, "Hello World!", 0xFFFF, 0x0000, 16, 0);
}

#endif
