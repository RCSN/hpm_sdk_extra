#include "WS2812.h"
#include <board.h>
#include <hpm_clock_drv.h>
#include <hpm_common.h>
#include <hpm_dma_mgr.h>
#include <hpm_dmamux_drv.h>
#include <hpm_dmav2_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include <hpm_gpiom_soc_drv.h>
#include <hpm_gptmr_drv.h>
#include <hpm_soc.h>
#include <string.h>


#if WS2812_USE_SPI
#include "hpm_spi.h"
#define  WS212_SPI_FREQ            (8000000UL)
#endif

#define _BUFFER_CONCAT3(x, y, z)     x ## y ## z
#define BUFFER_CONCAT3(x, y, z)     _BUFFER_CONCAT3(x, y, z)

typedef BUFFER_CONCAT3(uint, WS2812_BIT_TYPE_SIZE, _t)  buffer_type;

#if !WS2812_USE_SPI
static uint32_t _gptmr_freq = 0;
static buffer_type _bit0_pluse_width = 0;
static buffer_type _bit1_pluse_width = 0;
static const uint32_t _WS2812_Freq = 800000;
static const uint32_t _WS2812_DATA_WIDTH = DMA_MGR_TRANSFER_WIDTH_WORD;
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8)
static dma_linked_descriptor_t descriptors[WS2812_LED_NUM - 1];
static dma_resource_t dma_resource_pool;
#else
static buffer_type _bit0_pluse_width = 0xe0;  /* 0b11100000 40% high */
static buffer_type _bit1_pluse_width = 0xF8;  /* 0b11111000 60% high */
#endif


ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4)
static buffer_type WS2812_LED_Buffer
#if WS2812_LED_CONNECT == WS2812_CONNECT_LINE
    [WS2812_LED_NUM][24];
#elif WS2812_LED_CONNECT == WS2812_CONNECT_MATRIX
    [WS2812_LED_COL][WS2812_LED_ROW][24];
#elif WS2812_LED_CONNECT == WS2812_CONNECT_3D
    [WS2812_LED_COL][WS2812_LED_ROW][WS2812_LED_LAYER][24];
#endif

ATTR_PLACE_AT_NONCACHEABLE WS2812_LED_t WS2812_LED[WS2812_LED_NUM];

ATTR_PLACE_AT_NONCACHEABLE WS2812_RGB_t WS2812_Buffer[WS2812_LED_NUM];

static volatile bool dma_is_done = false;

#if !WS2812_USE_SPI
static void GPTMR_Init()
{
    gptmr_channel_config_t config;
    gptmr_channel_get_default_config(_WS2812_GPTMR_PTR, &config);
    gptmr_stop_counter(_WS2812_GPTMR_PTR, WS2812_GPTMR_CHANNLE);
    config.cmp_initial_polarity_high = true;
    config.dma_request_event = gptmr_dma_request_on_reload;
    config.reload = _gptmr_freq / _WS2812_Freq;
    config.cmp[0] = UINT32_MAX;
    config.enable_cmp_output = true;
    gptmr_channel_config(_WS2812_GPTMR_PTR, WS2812_GPTMR_CHANNLE, &config, false);
    gptmr_start_counter(_WS2812_GPTMR_PTR, WS2812_GPTMR_CHANNLE);

    dma_mgr_enable_channel(&dma_resource_pool);
}

#else
static void spi_init(void)
{
    spi_initialize_config_t init_config;
    clock_add_to_group(WS2812_SPI_CLCOK, 0);
    hpm_spi_get_default_init_config(&init_config);
    init_config.direction = msb_first;
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_odd_clk_edges;
    init_config.clk_polarity = spi_sclk_low_idle;
    init_config.data_len = WS2812_BIT_TYPE_SIZE;
    /* step.1  initialize spi */
    if (hpm_spi_initialize(WS2812_SPI, &init_config) != status_success) {
        printf("hpm_spi_initialize fail\n");
        while (1) {
        }
    }
}
#endif

/**
 * @brief 初始化LED连接方式，该函数为弱函数，库里仅提供默认的连接方式，用户可以自行实现
 */
ATTR_WEAK
void WS2812_LEDConnectInit(void)
{
#if WS2812_LED_CONNECT == WS2812_CONNECT_LINE
    // 默认情况下每一个灯都是首尾相连，链表中的第一个灯的数据是第一个灯的数据，最后一个灯的数据是第一个灯的数据
    for (size_t i = 0; i < WS2812_LED_NUM; i++)
    {
        WS2812_LED[i].buffer = &WS2812_LED_Buffer[i][0];
        if (i < WS2812_LED_NUM - 1)
        {
            WS2812_LED[i].next = &WS2812_LED[(i + 1) % WS2812_LED_NUM];
        }
    }
#elif WS2812_LED_CONNECT == WS2812_CONNECT_MATRIX
    // 矩阵连接方式，默认每一行的最后一个灯的数据是下一行的第一个灯的数据。
    // WS2812_LED 始终为一维数组，通过计算行和列的关系，将其映射到一维数组中
    for (int i = 0; i < WS2812_LED_COL; i++)
    {
        for (int j = 0; j < WS2812_LED_ROW; j++)
        {
            uint32_t index = i * WS2812_LED_ROW + j;
            WS2812_LED[index].buffer = WS2812_LED_Buffer[i][j];
            if (index < WS2812_LED_NUM - 1)
            {
                WS2812_LED[index].next = &WS2812_LED[index + 1];
            }
        }
    }
#elif WS2812_LED_CONNECT == WS2812_CONNECT_3D
    // 3D连接方式，默认每一层的最后一个灯的数据是下一层的第一个灯的数据。
    // WS2812_LED 始终为一维数组，通过计算行和列的关系，将其映射到一维数组中
    for (int i = 0; i < WS2812_LED_COL; i++)
    {
        for (int j = 0; j < WS2812_LED_ROW; j++)
        {
            for (int k = 0; k < WS2812_LED_LAYER; k++)
            {
                uint32_t index = i * WS2812_LED_ROW * WS2812_LED_LAYER + j * WS2812_LED_LAYER + k;
                WS2812_LED[index].buffer = WS2812_LED_Buffer[i][j][k];
                if (index < WS2812_LED_NUM - 1)
                {
                    WS2812_LED[index].next = &WS2812_LED[index + 1];
                }
            }
        }
    }
#endif
}

#if !WS2812_USE_SPI
void WS2812_DMA_Callback(DMAV2_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)channel;
    (void)user_data;

    dma_resource_t *resource = &dma_resource_pool;
    if (resource->channel == channel)
    {
        static uint32_t i = 0;
        i++;
        if (i == WS2812_LED_NUM)
        {
            gptmr_stop_counter(_WS2812_GPTMR_PTR, WS2812_GPTMR_CHANNLE);
            dma_mgr_disable_channel(resource);
            gptmr_clear_status(_WS2812_GPTMR_PTR, GPTMR_CH_RLD_STAT_MASK(WS2812_GPTMR_CHANNLE));

            HPM_IOC->PAD[_WS2812_DIN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
            gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(_WS2812_DIN_PIN), GPIO_GET_PIN_INDEX(_WS2812_DIN_PIN), 0);

            dma_is_done = true;
            i = 0;
        }
    }
}

void DMA_Init()
{
    dma_mgr_chn_conf_t ch_config;
    dma_resource_t *resource = NULL;

    resource = &dma_resource_pool;
    dma_mgr_get_default_chn_config(&ch_config);

    for (int i = 0; i < WS2812_LED_NUM - 1; i++)
    {
        ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&WS2812_LED[i + 1].buffer[0]);
        ch_config.dst_addr = (uint32_t)&_WS2812_GPTMR_PTR->CHANNEL[WS2812_GPTMR_CHANNLE].CMP[0];
        ch_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        ch_config.src_width = _WS2812_DATA_WIDTH;
        ch_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        ch_config.src_burst_size = DMA_MGR_NUM_TRANSFER_PER_BURST_1T;
        ch_config.dst_width = _WS2812_DATA_WIDTH;
        ch_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
        ch_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        ch_config.size_in_byte = 96;
        ch_config.priority = DMA_MGR_CHANNEL_PRIORITY_HIGH;
        ch_config.en_dmamux = true;
        ch_config.dmamux_src = _WS2812_DMAMUX_SRC;
        if (i == (WS2812_LED_NUM - 2))
        {
            ch_config.linked_ptr = 0;
        }
        else
        {
            ch_config.linked_ptr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&descriptors[i + 1]);
        }
        if (status_success !=
            dma_mgr_config_linked_descriptor(resource, &ch_config, (dma_mgr_linked_descriptor_t *)&descriptors[i]))
        {
            printf("generate dma desc fail\n");
            return;
        }
        descriptors[i].ctrl &= ~DMA_MGR_INTERRUPT_MASK_TC;
    }

    dma_mgr_get_default_chn_config(&ch_config);
    //    ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t) WS2812_LED[0].buffer);
    ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&WS2812_LED[0].buffer[0]);
    ch_config.dst_addr = (uint32_t)&_WS2812_GPTMR_PTR->CHANNEL[WS2812_GPTMR_CHANNLE].CMP[0];
    ch_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
    ch_config.src_width = _WS2812_DATA_WIDTH;
    ch_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
    ch_config.src_burst_size = DMA_MGR_NUM_TRANSFER_PER_BURST_1T;
    ch_config.dst_width = _WS2812_DATA_WIDTH;
    ch_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_FIXED;
    ch_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
    ch_config.size_in_byte = 96;
    ch_config.priority = DMA_MGR_CHANNEL_PRIORITY_HIGH;
    ch_config.en_dmamux = true;
    ch_config.dmamux_src = _WS2812_DMAMUX_SRC;
#if WS2812_LED_NUM == 1
    ch_config.linked_ptr = 0;
#else
    ch_config.linked_ptr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&descriptors[0]);

#endif
    if (status_success != dma_mgr_setup_channel(resource, &ch_config))
    {
        printf("DMA setup channel failed\n");
        return;
    }
    //    dma_mgr_setup_channel(resource, &ch_config);
    dma_mgr_install_chn_tc_callback(resource, WS2812_DMA_Callback, NULL);
    dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
    dma_mgr_enable_dma_irq_with_priority(resource, 1);
}
#else
void spi_txdma_complete_callback(uint32_t channel)
{
    (void)channel;
    dma_is_done = true;
}
#endif

void WS2812_Init(void)
{
#if !WS2812_USE_SPI
    HPM_IOC->PAD[_WS2812_DIN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0); // 初始化GPIO
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(_WS2812_DIN_PIN), GPIO_GET_PIN_INDEX(_WS2812_DIN_PIN),
                             gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(_WS2812_DIN_PIN), GPIO_GET_PIN_INDEX(_WS2812_DIN_PIN));
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(_WS2812_DIN_PIN), GPIO_GET_PIN_INDEX(_WS2812_DIN_PIN), 0);

    _gptmr_freq = clock_get_frequency(_WS2812_GPTMR_NAME);  // 获取GPTMR的频率
    _bit0_pluse_width = _gptmr_freq / _WS2812_Freq / 3;     // 0的脉冲宽度
    _bit1_pluse_width = _gptmr_freq / _WS2812_Freq * 2 / 3; // 1的脉冲宽度
#else
    HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_SPI1_MOSI;
    /* step.1  initialize spi */
    spi_init();
    /* step.2  set spi sclk frequency for master */
    if (hpm_spi_set_sclk_frequency(WS2812_SPI, WS212_SPI_FREQ) != status_success) {
        printf("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }

    /* step.3 install dma callback if want use dma */
    if (hpm_spi_dma_install_callback(WS2812_SPI, spi_txdma_complete_callback, NULL) != status_success) {
        printf("hpm_spi_dma_install_callback fail\n");
        while (1) {
        }
    }
#endif
    WS2812_LEDConnectInit();
    for (int i = 0; i < WS2812_LED_NUM; i++)
    {
        WS2812_SetPixel(i, 0, 0, 0);
    }

    for (int i = 0; i < WS2812_LED_NUM; i++)
    {
        for (int j = 0; j < 24; j++)
        {
#if !WS2812_USE_SPI
            WS2812_LED_Buffer[i][j] = (sizeof(buffer_type) == 1) ? UINT8_MAX : UINT32_MAX;
#else
            WS2812_LED_Buffer[i][j] = 0;
#endif
        }
    }
#if !WS2812_USE_SPI
    HPM_IOC->PAD[_WS2812_DIN_PIN].FUNC_CTL = _WS2812_DIN_FUNC; // 初始化GPIO
    dma_mgr_request_resource(&dma_resource_pool);
    DMA_Init();   // 初始化DMA
    GPTMR_Init(); // 初始化GPTMR
#else

#endif
}

void WS2812_Update(bool blocking)
{
    for (int index = 0; index < WS2812_LED_NUM; index++)
    {
        buffer_type *buf = &WS2812_LED_Buffer[index][0];
        // GRB
        for (int i = 0; i < 8; i++)
        {
#if !WS2812_USE_SPI
            if (WS2812_Buffer[index].g & (1 << (7 - i)))
            {
                buf[i] = _bit0_pluse_width;
            }
            else
            {
                buf[i] = _bit1_pluse_width;
            }

            if (WS2812_Buffer[index].r & (1 << (7 - i)))
            {
                buf[i + 8] = _bit0_pluse_width;
            }
            else
            {
                buf[i + 8] = _bit1_pluse_width;
            }

            if (WS2812_Buffer[index].b & (1 << (7 - i)))
            {
                buf[i + 16] = _bit0_pluse_width;
            }
            else
            {
                buf[i + 16] = _bit1_pluse_width;
            }
#else
            WS2812_RGB_t rgb = WS2812_Buffer[index];
            buf[i]  = (rgb.g & 0x01) ? _bit1_pluse_width : _bit0_pluse_width;
            buf[i + 8] = (rgb.r & 0x01) ? _bit1_pluse_width : _bit0_pluse_width;
            buf[i + 16] = (rgb.b & 0x01) ? _bit1_pluse_width : _bit0_pluse_width;
            rgb.r >>= 1;
            rgb.g >>= 1;
            rgb.b >>= 1;
#endif
        }
//        printf("index: %d, r: %d, g: %d, b: %d\n", index, WS2812_Buffer[index].r, WS2812_Buffer[index].g, WS2812_Buffer[index].b);
    }
#if !WS2812_USE_SPI
    if (blocking == true) {
        while (!dma_is_done) {
        };
    }
    dma_is_done = false;

    DMA_Init();                                                // 每次都重新配置一次
    HPM_IOC->PAD[_WS2812_DIN_PIN].FUNC_CTL = _WS2812_DIN_FUNC; // 初始化GPIO

    dma_mgr_enable_channel(&dma_resource_pool);
    gptmr_start_counter(_WS2812_GPTMR_PTR, WS2812_GPTMR_CHANNLE);
#else
    if (hpm_spi_transmit_nonblocking(WS2812_SPI, (buffer_type *)&WS2812_LED_Buffer[0][0], WS2812_LED_NUM * 24) != status_success) {
        printf("hpm_spi_transmit_receive_nonblocking fail\n");
        return;
    }
    if (blocking == true) {
        while (!dma_is_done) {
        };
        dma_is_done = false;
    }
#endif
}

void WS2812_SetPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812_LED_NUM)
    {
        return;
    }
    WS2812_Buffer[index].r = r;
    WS2812_Buffer[index].g = g;
    WS2812_Buffer[index].b = b;
}

bool WS2812_IsBusy(void)
{
    return !dma_is_done;
}

void WS2812_MixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812_LED_NUM)
    {
        return;
    }
    WS2812_Buffer[index].r = (WS2812_Buffer[index].r + r) / 2;
    WS2812_Buffer[index].g = (WS2812_Buffer[index].g + g) / 2;
    WS2812_Buffer[index].b = (WS2812_Buffer[index].b + b) / 2;
}

void WS2812_ReverseMixPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= WS2812_LED_NUM)
    {
        return;
    }
    uint8_t temp;
    temp = (WS2812_Buffer[index].r * 2);
    if (temp > r)
    {
        WS2812_Buffer[index].r = temp - r;
    }
    else
    {
        WS2812_Buffer[index].r = 0;
    }

    temp = (WS2812_Buffer[index].g * 2);
    if (temp > g)
    {
        WS2812_Buffer[index].g = temp - g;
    }
    else
    {
        WS2812_Buffer[index].g = 0;
    }

    temp = (WS2812_Buffer[index].b * 2);
    if (temp > b)
    {
        WS2812_Buffer[index].b = temp - b;
    }
    else
    {
        WS2812_Buffer[index].b = 0;
    }
}

void WS2812_Clear_Busy(void)
{
    dma_is_done = false;
}
