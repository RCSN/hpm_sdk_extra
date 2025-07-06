/*
* Copyright (c) 2025 HPMicro
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/
#if defined(ENABLE_NV3007_LCD_DRIVER) && (ENABLE_NV3007_LCD_DRIVER == 1)

#include "nv3007_display.h"
#include "hpm_spi.h"
#include "hpm_spi_drv.h"
#include "board.h"
#include "lcd_font.h"

static spi_tft_lcd_context_t *nv3007_lcd_ctx;

static void nv3007_display_wr_reg(uint8_t dat)
{
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_set_dc(nv3007_lcd_ctx, false);
    spi_tft_lcd_transfer_data_blocking(nv3007_lcd_ctx, 8, (uint8_t *)&dat, 1, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
    spi_tft_lcd_set_dc(nv3007_lcd_ctx, true);
}

static void nv3007_display_wr_data8(uint8_t dat)
{
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(nv3007_lcd_ctx, 8, (uint8_t *)&dat, 1, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
}

static void nv3007_display_wr_data16(uint16_t dat)
{
#if 0
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(&nv3007_lcd_ctx, 16, (uint8_t *)&dat, 2, SPI_TFT_LCD_POLL_DEFAULT_TIMEOUT);
#else
    nv3007_display_wr_data8(dat >> 8);
    nv3007_display_wr_data8(dat);
#endif
}

static void nv3007_display_write_ram_blocking(uint8_t bit_width, uint8_t *data, uint32_t size, uint32_t timeout)
{
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_blocking(nv3007_lcd_ctx, bit_width, data, size, timeout);
}

static void nv3007_display_write_ram_nonblocking(uint8_t bit_width, uint8_t *data, uint32_t size)
{
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_transfer_data_nonblocking(nv3007_lcd_ctx, bit_width, data, size);
}

static void nv3007_display_cmd_init(void)
{
    if (nv3007_lcd_ctx == NULL) {
        return;
    }
    spi_tft_lcd_set_rst(nv3007_lcd_ctx, true);
    board_delay_ms(50);
    spi_tft_lcd_set_rst(nv3007_lcd_ctx, false);
    board_delay_ms(50);
    spi_tft_lcd_set_rst(nv3007_lcd_ctx, true);
    board_delay_ms(120);

    //NV3006A1N IVO2.6 
    nv3007_display_wr_reg(0xff);
    nv3007_display_wr_data8(0xa5);	
    nv3007_display_wr_reg(0x9a);
    nv3007_display_wr_data8(0x08);
    nv3007_display_wr_reg(0x9b);
    nv3007_display_wr_data8(0x08);	
    nv3007_display_wr_reg(0x9c);
    nv3007_display_wr_data8(0xb0);	
    nv3007_display_wr_reg(0x9d);
    nv3007_display_wr_data8(0x16);
    nv3007_display_wr_reg(0x9e);
    nv3007_display_wr_data8(0xc4);
    nv3007_display_wr_reg(0x8f);
    nv3007_display_wr_data8(0x55);
    nv3007_display_wr_data8(0x04);
    nv3007_display_wr_reg(0x84);
    nv3007_display_wr_data8(0x90);
    nv3007_display_wr_reg(0x83);
    nv3007_display_wr_data8(0x7b);
    nv3007_display_wr_reg(0x85);
    nv3007_display_wr_data8(0x33);
    nv3007_display_wr_reg(0x60);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x70);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x61);
    nv3007_display_wr_data8(0x02);
    nv3007_display_wr_reg(0x71);
    nv3007_display_wr_data8(0x02);
    nv3007_display_wr_reg(0x62);
    nv3007_display_wr_data8(0x04);
    nv3007_display_wr_reg(0x72);
    nv3007_display_wr_data8(0x04);
    nv3007_display_wr_reg(0x6c);
    nv3007_display_wr_data8(0x29);
    nv3007_display_wr_reg(0x7c);
    nv3007_display_wr_data8(0x29);
    nv3007_display_wr_reg(0x6d);
    nv3007_display_wr_data8(0x31);
    nv3007_display_wr_reg(0x7d);
    nv3007_display_wr_data8(0x31);
    nv3007_display_wr_reg(0x6e);
    nv3007_display_wr_data8(0x0f);
    nv3007_display_wr_reg(0x7e);
    nv3007_display_wr_data8(0x0f);
    nv3007_display_wr_reg(0x66);
    nv3007_display_wr_data8(0x21);
    nv3007_display_wr_reg(0x76);
    nv3007_display_wr_data8(0x21);
    nv3007_display_wr_reg(0x68);
    nv3007_display_wr_data8(0x3A);
    nv3007_display_wr_reg(0x78);
    nv3007_display_wr_data8(0x3A);
    nv3007_display_wr_reg(0x63);
    nv3007_display_wr_data8(0x07);
    nv3007_display_wr_reg(0x73);
    nv3007_display_wr_data8(0x07);
    nv3007_display_wr_reg(0x64);
    nv3007_display_wr_data8(0x05);
    nv3007_display_wr_reg(0x74);
    nv3007_display_wr_data8(0x05);
    nv3007_display_wr_reg(0x65);
    nv3007_display_wr_data8(0x02);
    nv3007_display_wr_reg(0x75);
    nv3007_display_wr_data8(0x02);
    nv3007_display_wr_reg(0x67);
    nv3007_display_wr_data8(0x23);
    nv3007_display_wr_reg(0x77);
    nv3007_display_wr_data8(0x23);
    nv3007_display_wr_reg(0x69);
    nv3007_display_wr_data8(0x08);
    nv3007_display_wr_reg(0x79);
    nv3007_display_wr_data8(0x08);
    nv3007_display_wr_reg(0x6a);
    nv3007_display_wr_data8(0x13);
    nv3007_display_wr_reg(0x7a);
    nv3007_display_wr_data8(0x13);
    nv3007_display_wr_reg(0x6b);
    nv3007_display_wr_data8(0x13);
    nv3007_display_wr_reg(0x7b);
    nv3007_display_wr_data8(0x13);
    nv3007_display_wr_reg(0x6f);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x7f);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x50);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x52);
    nv3007_display_wr_data8(0xd6);
    nv3007_display_wr_reg(0x53);
    nv3007_display_wr_data8(0x08);
    nv3007_display_wr_reg(0x54);
    nv3007_display_wr_data8(0x08);
    nv3007_display_wr_reg(0x55);
    nv3007_display_wr_data8(0x1e);
    nv3007_display_wr_reg(0x56);
    nv3007_display_wr_data8(0x1c);
    //goa map_sel
    nv3007_display_wr_reg(0xa0);
    nv3007_display_wr_data8(0x2b);
    nv3007_display_wr_data8(0x24);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xa1);
    nv3007_display_wr_data8(0x87);
    nv3007_display_wr_reg(0xa2);
    nv3007_display_wr_data8(0x86);
    nv3007_display_wr_reg(0xa5);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xa6);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xa7);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xa8);
    nv3007_display_wr_data8(0x36);
    nv3007_display_wr_reg(0xa9);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xaa);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xB9);
    nv3007_display_wr_data8(0x85);
    nv3007_display_wr_reg(0xBA);
    nv3007_display_wr_data8(0x84);
    nv3007_display_wr_reg(0xBB);
    nv3007_display_wr_data8(0x83);
    nv3007_display_wr_reg(0xBC);
    nv3007_display_wr_data8(0x82);
    nv3007_display_wr_reg(0xBD);
    nv3007_display_wr_data8(0x81);
    nv3007_display_wr_reg(0xBE);
    nv3007_display_wr_data8(0x80);
    nv3007_display_wr_reg(0xBF);
    nv3007_display_wr_data8(0x01);
    nv3007_display_wr_reg(0xC0);
    nv3007_display_wr_data8(0x02);
    nv3007_display_wr_reg(0xc1);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xc2);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xc3);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xc4);
    nv3007_display_wr_data8(0x33);
    nv3007_display_wr_reg(0xc5);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xc6);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xC8);
    nv3007_display_wr_data8(0x33);
    nv3007_display_wr_data8(0x33);
    nv3007_display_wr_reg(0xC9);
    nv3007_display_wr_data8(0x68);
    nv3007_display_wr_reg(0xCA);
    nv3007_display_wr_data8(0x69);
    nv3007_display_wr_reg(0xCB);
    nv3007_display_wr_data8(0x6a);
    nv3007_display_wr_reg(0xCC);
    nv3007_display_wr_data8(0x6b);
    nv3007_display_wr_reg(0xCD);
    nv3007_display_wr_data8(0x33);
    nv3007_display_wr_data8(0x33); 
    nv3007_display_wr_reg(0xCE);
    nv3007_display_wr_data8(0x6c);
    nv3007_display_wr_reg(0xCF);
    nv3007_display_wr_data8(0x6d);
    nv3007_display_wr_reg(0xD0);
    nv3007_display_wr_data8(0x6e);
    nv3007_display_wr_reg(0xD1);
    nv3007_display_wr_data8(0x6f);
    nv3007_display_wr_reg(0xAB);
    nv3007_display_wr_data8(0x03);
    nv3007_display_wr_data8(0x67);
    nv3007_display_wr_reg(0xAC);
    nv3007_display_wr_data8(0x03);
    nv3007_display_wr_data8(0x6b);
    nv3007_display_wr_reg(0xAD);
    nv3007_display_wr_data8(0x03);
    nv3007_display_wr_data8(0x68);
    nv3007_display_wr_reg(0xAE);
    nv3007_display_wr_data8(0x03);
    nv3007_display_wr_data8(0x6c);
    nv3007_display_wr_reg(0xb3);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xb4);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xb5);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xB6);
    nv3007_display_wr_data8(0x32);
    nv3007_display_wr_reg(0xB7);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xB8);
    nv3007_display_wr_data8(0x7e);
    nv3007_display_wr_reg(0xe0);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0xe1);
    nv3007_display_wr_data8(0x03);
    nv3007_display_wr_data8(0x0f);
    nv3007_display_wr_reg(0xe2);
    nv3007_display_wr_data8(0x04);
    nv3007_display_wr_reg(0xe3);
    nv3007_display_wr_data8(0x01);
    nv3007_display_wr_reg(0xe4);
    nv3007_display_wr_data8(0x0e);
    nv3007_display_wr_reg(0xe5);
    nv3007_display_wr_data8(0x01);
    nv3007_display_wr_reg(0xe6);
    nv3007_display_wr_data8(0x19);
    nv3007_display_wr_reg(0xe7);
    nv3007_display_wr_data8(0x10);
    nv3007_display_wr_reg(0xe8);
    nv3007_display_wr_data8(0x10);
    nv3007_display_wr_reg(0xea);
    nv3007_display_wr_data8(0x12);
    nv3007_display_wr_reg(0xeb);
    nv3007_display_wr_data8(0xd0);
    nv3007_display_wr_reg(0xec);
    nv3007_display_wr_data8(0x04);
    nv3007_display_wr_reg(0xed);
    nv3007_display_wr_data8(0x07);
    nv3007_display_wr_reg(0xee);
    nv3007_display_wr_data8(0x07);
    nv3007_display_wr_reg(0xef);
    nv3007_display_wr_data8(0x09);
    nv3007_display_wr_reg(0xf0);
    nv3007_display_wr_data8(0xd0);
    nv3007_display_wr_reg(0xf1);
    nv3007_display_wr_data8(0x0e);

    //nv3007_display_wr_reg(0xF9);
    //nv3007_display_wr_data8(0x17); 
    //nv3007_display_wr_reg(0xf2);
    //nv3007_display_wr_data8(0x2e);
    //nv3007_display_wr_data8(0x1b);
    //nv3007_display_wr_data8(0x0b);
    //nv3007_display_wr_data8(0x20);
    //nv3007_display_wr_reg(0xF9);
    nv3007_display_wr_data8(0x17); 
    nv3007_display_wr_reg(0xf2);
    nv3007_display_wr_data8(0x2c);
    nv3007_display_wr_data8(0x1b);
    nv3007_display_wr_data8(0x0b);
    nv3007_display_wr_data8(0x20);
    ////1 dot
    nv3007_display_wr_reg(0xe9);
    nv3007_display_wr_data8(0x29);
    nv3007_display_wr_reg(0xec);
    nv3007_display_wr_data8(0x04);
    //TE
    nv3007_display_wr_reg(0x35);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x44);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_data8(0x10);
    nv3007_display_wr_reg(0x46);
    nv3007_display_wr_data8(0x10);
    nv3007_display_wr_reg(0xff);
    nv3007_display_wr_data8(0x00);
    nv3007_display_wr_reg(0x3a);
    nv3007_display_wr_data8(0x05);
    nv3007_display_wr_reg(0x36);
    if (USE_HORIZONTIAL == 0)
    {
        nv3007_display_wr_data8(0x00);
    } else if (USE_HORIZONTIAL == 1) {
        nv3007_display_wr_data8(0xC0);
    } else if (USE_HORIZONTIAL == 2) {
        nv3007_display_wr_data8(0x70);
    } else {
        nv3007_display_wr_data8(0xA0);
    }
    nv3007_display_wr_reg(0x11); 
    board_delay_ms(220); 
    nv3007_display_wr_reg(0x29); 
    board_delay_ms(200);
}


void nv3007_display_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if USE_HORIZONTIAL==0
    nv3007_display_wr_reg(0x2a); /* 列地址设置 */
    nv3007_display_wr_data16(x1+0x0C);
    nv3007_display_wr_data16(x2+0x0C);
    nv3007_display_wr_reg(0x2b); /* 行地址设置 */
    nv3007_display_wr_data16(y1);
    nv3007_display_wr_data16(y2);
    nv3007_display_wr_reg(0x2c); /* 储存器写 */
#elif USE_HORIZONTIAL==1
    nv3007_display_wr_reg(0x2a); /* 列地址设置 */
    nv3007_display_wr_data16(x1+0x0E);
    nv3007_display_wr_data16(x2+0x0E);
    nv3007_display_wr_reg(0x2b); /* 行地址设置 */
    nv3007_display_wr_data16(y1);
    nv3007_display_wr_data16(y2);
    nv3007_display_wr_reg(0x2c); /* 储存器写 */
#elif USE_HORIZONTIAL==2
    nv3007_display_wr_reg(0x2a); /* 列地址设置 */
    nv3007_display_wr_data16(x1);
    nv3007_display_wr_data16(x2);
    nv3007_display_wr_reg(0x2b); /* 行地址设置 */
    nv3007_display_wr_data16(y1+0x0E);
    nv3007_display_wr_data16(y2+0x0E);
    nv3007_display_wr_reg(0x2c); /* 储存器写 */
#else
    nv3007_display_wr_reg(0x2a); /* 列地址设置 */
    nv3007_display_wr_data16(x1);
    nv3007_display_wr_data16(x2);
    nv3007_display_wr_reg(0x2b); /* 行地址设置 */
    nv3007_display_wr_data16(y1+0x0C);
    nv3007_display_wr_data16(y2+0x0C);
    nv3007_display_wr_reg(0x2c); /* 储存器写 */
#endif
}


void nv3007_display_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t i, j;
    nv3007_display_address_set(x1, y1, x2 - 1, y2 - 1);
    for (j = y1; j < y2; j++) {
        for (i = x1; i < x2; i++) {
            nv3007_display_wr_data16(color);
        }
    }
}

void Lnv3007_display_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    nv3007_display_address_set(x, y, x, y);
    nv3007_display_wr_data16(color);
}

void nv3007_display_show_char(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum; // 一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' ';                                     // 得到偏移后的值
    nv3007_display_address_set(x, y, x + sizex - 1, y + sizey - 1); // 设置显示窗口
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
                if (temp & (0x01 << t))
                    nv3007_display_wr_data16(fc);
                else
                    nv3007_display_wr_data16(bc);
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
                    Lnv3007_display_draw_point(x, y, fc); // 画一个点
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

void nv3007_display_show_string(uint16_t x, uint16_t y,const char *s, uint16_t fc, uint16_t bc, uint16_t sizey, uint8_t mode)
{
    while ((*s <= '~') && (*s >= ' ')) // 判断是不是非法字符
    {
        if (x > (NV3007_LCD_H_RES - 1) || y > (NV3007_LCD_V_RES - 1))
            return;
        nv3007_display_show_char(x, y, *s, fc, bc, sizey, mode);
        x += sizey / 2;
        s++;
    }
}

void nv3007_display_init(spi_tft_lcd_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    nv3007_lcd_ctx = ctx;
    ctx->address_set = nv3007_display_address_set;
    ctx->write_ram_blocking = nv3007_display_write_ram_blocking;
    ctx->write_ram_nonblocking = nv3007_display_write_ram_nonblocking;
    ctx->fill_color = nv3007_display_fill_color;
    ctx->draw_point = Lnv3007_display_draw_point;
    ctx->width = NV3007_LCD_H_RES;
    ctx->height = NV3007_LCD_V_RES;
    ctx->pixel_in_byte = 2;

    spi_tft_lcd_hardware_init(ctx);
    nv3007_display_cmd_init();
    // nv3007_display_fill_color(0, 0, NV3007_LCD_H_RES, NV3007_LCD_V_RES, 0xFFFF);
    // board_delay_ms(100);
    nv3007_display_fill_color(0, 0, NV3007_LCD_H_RES, NV3007_LCD_V_RES, 0xFF00);
    board_delay_ms(100);
    // nv3007_display_fill_color(0, 0, NV3007_LCD_H_RES, NV3007_LCD_V_RES, 0x00FF);
    // board_delay_ms(100);
    nv3007_display_fill_color(0, 0, NV3007_LCD_H_RES, NV3007_LCD_V_RES, 0x0000);
    board_delay_ms(100);
    nv3007_display_show_string(0, 0, "Hello World!", 0xFFFF, 0x0000, 16, 0);
}

#endif
