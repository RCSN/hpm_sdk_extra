#ifndef WS2812_H
#define WS2812_H

#include "WS2812_Conf.h"
#include <stdbool.h>

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y)        CONCATENATE_DETAIL(x, y)
#define STRINGIFY_DETAIL(x)      #x

#if  !WS2812_USE_SPI
#define _WS2812_DIN_PIN    CONCATENATE(IOC_PAD_, WS2812_DIN)
#define _WS2812_DIN_FUNC   CONCATENATE(CONCATENATE(CONCATENATE(CONCATENATE(CONCATENATE(IOC_, WS2812_DIN), _FUNC_CTL_GPTMR), WS2812_GPTMR), _COMP_), WS2812_GPTMR_CHANNLE)
#define _WS2812_GPTMR_NAME CONCATENATE(clock_gptmr, WS2812_GPTMR)
#define _WS2812_GPTMR_PTR  CONCATENATE(HPM_GPTMR, WS2812_GPTMR)
#define _WS2812_DMAMUX_SRC CONCATENATE(CONCATENATE(CONCATENATE(HPM_DMA_SRC_GPTMR, WS2812_GPTMR), _), WS2812_GPTMR_CHANNLE)
#else

#endif

void WS2812_Init(void);
void WS2812_Update(bool blocking);
bool WS2812_IsBusy(void);
void WS2812_Clear_Busy(void);

void WS2812_SetPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 将现在的RGB值混合之前的RGB值
 *
 * @param index 灯珠索引
 * @param r 红色
 * @param g 绿色
 * @param b 蓝色
 */
void WS2812_MixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 将现在的RGB值逆混合之前的RGB值
 *
 * @param index 灯珠索引
 * @param r 红色
 * @param g 绿色
 * @param b 蓝色
 */
void WS2812_ReverseMixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

#endif // WS2812_H
