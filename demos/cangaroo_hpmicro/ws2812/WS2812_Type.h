#ifndef WS2812_TYPE_H
#define WS2812_TYPE_H

#include <stdint.h>

/**
 * @brief WS2812 灯珠连接方式
 *
 */
#define WS2812_CONNECT_LINE 0
#define WS2812_CONNECT_MATRIX 1
#define WS2812_CONNECT_3D 2

#if !WS2812_USE_SPI
#define  WS2812_BIT_TYPE_SIZE      32
#else
#define  WS2812_BIT_TYPE_SIZE      8
#endif

#define _BUFFER_CONCAT3(x, y, z)     x ## y ## z
#define BUFFER_CONCAT3(x, y, z)     _BUFFER_CONCAT3(x, y, z)

typedef BUFFER_CONCAT3(uint, WS2812_BIT_TYPE_SIZE, _t)  buffer_type;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} WS2812_RGB_t;

/**
 * @brief WS2812 灯珠链表，其中有一个指向灯珠buffer的地址和下一个灯珠的链表
 */
typedef struct WS2812_LED {
    buffer_type *buffer;
    struct WS2812_LED *next;
} WS2812_LED_t;

#endif //WS2812_TYPE_H
