#ifndef WS2812_H
#define WS2812_H

#include "WS2812_Conf.h"
#include <stdbool.h>
#include <stdint.h>
#include "ws2812_type.h"

#ifndef WS2812_DEBUG
#define WS2812_DEBUG(...)
#endif

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

/**
 * @brief Initialize the RGB
 *
 */
void WS2812_Init(void);

/**
 * @brief Update the RGB buffer
 *
 * @param blocking whether to wait for the update to complete
 */
void WS2812_Update(bool blocking);

/**
 * @brief Check if the RGB is busy
 *
 * @return true RGB is busy
 * @return false RGB is not busy
 */
bool WS2812_IsBusy(void);

/**
 * @brief Clear the busy flag
 *
 */
void WS2812_Clear_Busy(void);

/**
 * @brief Set the RGB value of the specified RGB index
 *
 * @param index RGB index
 * @param r red
 * @param g green
 * @param b blue
 */
void WS2812_SetPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Mix the current RGB value with the previous RGB value
 *
 * @param index RGB index
 * @param r red
 * @param g green
 * @param b blue
 */
void WS2812_MixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief nversely mix the current RGB value with the previous RGB value
 *
 * @param index RGB index
 * @param r red
 * @param g green
 * @param b blue
 */
void WS2812_ReverseMixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

#endif // WS2812_H
