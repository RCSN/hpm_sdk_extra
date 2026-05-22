# VL53L5CX + WS2812 手势互动示例

## 概述

本项目在 hpm5301evklite 开发板上实现双 VL53L5CX ToF 传感器 + 16×16 WS2812 LED 点阵的手势互动演示。

VL53L5CX 是 ST 推出的 8×8 多区 ToF 测距传感器，单颗可提供 64 个独立测距区域。本示例使用两颗 VL53L5CX，分别挂载在 I2C2 和 I2C3 两条独立总线上（无需修改传感器 I2C 地址），合计提供 128 个测距区域，覆盖一个手掌大小的交互空间。

16×16 WS2812 LED 点阵通过 SPI1 的 MOSI 信号线驱动，使用 HPM SDK 的 SPI 组件以 DMA 方式传输数据。每个 ToF 测距区域映射为 2×2 LED 像素，实现距离数据的实时可视化。

本示例提供三种互动模式：深度热力图、手势灯光跟随、涟漪扩散效果，并内置手势识别引擎，支持左/右/上/下挥、推/拉、悬停等手势检测。

## 硬件连接

### 目标板

- **开发板**：hpm5301evklite
- **MCU**：HPM5301 (RISC-V, rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs)

### 引脚分配

| 外设 | 引脚 | 功能 | 说明 |
|------|------|------|------|
| I2C2 | PB08 | SCL | VL53L5CX 传感器 A 时钟线 |
| I2C2 | PB09 | SDA | VL53L5CX 传感器 A 数据线 |
| I2C3 | PB13 | SCL | VL53L5CX 传感器 B 时钟线 |
| I2C3 | PB12 | SDA | VL53L5CX 传感器 B 数据线 |
| SPI1 | PA29 | MOSI | WS2812 DIN 数据输入 |

### 连接说明

- **双传感器无需改地址**：两个 VL53L5CX 分别位于不同的 I2C 总线上，均使用默认 7 位地址 0x29（8 位地址 0x52），无需对传感器做 LPn 引脚控制来修改地址。
- **WS2812 仅需 MOSI**：SPI 模式下只需一根 MOSI 信号线，8MHz SPI 时钟，每个 WS2812 数据 bit 用 3 个 SPI bit 编码（`0xE0` 表示 0，`0xF8` 表示 1）。
- **蛇形布线**：16×16 矩阵采用蛇形连接，偶数行从左到右、奇数行从右到左，代码中做了相应处理。

## 功能模式

通过串口终端（115200 8N1）输入字符切换模式：

| 按键 | 模式 | 说明 |
|------|------|------|
| `1` | 深度热力图 | 距离 → HSV 色带映射，近=暖色/远=冷色 |
| `2` | 手势灯光跟随 | 手的质心位置 → LED 光标，挥手换色 |
| `3` | 涟漪效果 | 手出现时触发 2D 涟漪扩散动画 |
| `b` | 捕获背景 | 记录当前帧为背景参考（无手时操作） |
| `r` | 重置传感器 | 停止测距并重新初始化传感器 |

### 模式 1：深度热力图

将两个 VL53L5CX 的 8×8 距离数据映射到 16×16 LED 矩阵上：

- **传感器 A** → 点阵左半部分（列 0-7）
- **传感器 B** → 点阵右半部分（列 8-15）
- 每个 ToF 测区 → 2×2 LED 像素

距离到颜色的映射逻辑：
- 距离范围：30mm ~ 1500mm
- 近距离（30mm）→ 色相 200（暖色：红/橙/黄），直观表示"热"
- 远距离（1500mm）→ 色相 0（冷色：蓝/紫），直观表示"冷"
- 超出范围 → 黑色（熄灭）
- 超远距离（> 1200mm）→ 亮度逐渐衰减

HSV → RGB 转换使用纯整数运算，无浮点开销，适合 MCU 实时运行。

### 模式 2：手势灯光跟随

实时追踪手掌的质心位置，在 LED 矩阵上显示一个跟随光标：

- **光标结构**：
  - 中心（半径 < 2）：满亮度 255，饱和度 255
  - 辉光（半径 2-5）：亮度线性衰减，饱和度 200
  - 拖尾效果：每帧对现有像素做 `MixPixel(0,0,0)` 混合黑，实现渐隐拖尾

- **颜色映射**：
  - 基础色相 = 距离偏移（近=暖，远=冷）
  - 左挥 → 色相 -30
  - 右挥 → 色相 +30
  - 不断挥手可遍历整个色相环

- **坐标映射**：质心坐标 (0.0~7.0) × 2 → LED 坐标 (0.0~15.0)

### 模式 3：涟漪效果

手进入检测区域时，在手的位置触发一个 2D 涟漪扩散动画：

- **涟漪参数**：
  - 最多 4 个涟漪同时存在（环形缓冲区）
  - 最大半径 12（超过后自动销毁）
  - 扩散速度：0.3 / 帧
  - 环厚度：2.0 像素
  - 每个新涟漪色相递增 40，形成彩虹色序列

- **绘制逻辑**：
  - 计算每个 LED 到涟漪中心的距离 `dist`
  - 环距离 `ring_dist = |dist - radius|`
  - 当 `ring_dist < RIPPLE_WIDTH` 时，该像素在环上
  - 强度 = `1 - ring_dist / RIPPLE_WIDTH`（环中心最亮，边缘渐弱）
  - 亮度随半径扩大而衰减：`brightness × (1 - radius / max_radius)`
  - 多个涟漪使用 `MixPixel` 加法混合

- **手位置指示**：手存在时，在质心对应位置显示一个灰色 (80,80,80) 小点

## 手势识别引擎

手势引擎 (`gesture_engine.c/h`) 实现质心追踪 + 速度状态机的手势检测。

### 核心数据结构

```
centroid_t           - 每帧质心：cx/cy 位置、avg_distance 平均距离、present 是否存在、area 覆盖区域数
sensor_tracker_t     - 传感器追踪器：轨迹缓冲、前一帧质心、状态机状态、确认计数、速度向量
gesture_event_t      - 手势事件：手势类型、速度、质心位置、平均距离
gesture_engine_t     - 引擎上下文：双传感器配置、双测距结果、双追踪器、背景参考
```

### 质心计算

对每个传感器的 8×8=64 个测区：

1. **过滤**：跳过无效读数（距离 ≤ 30mm 或 ≥ 1500mm）
2. **背景减除**：如果已捕获背景，跳过与背景差异 < 100mm 的区域（排除静态物体）
3. **加权质心**：权重 = `(1500 - distance) / 1500`，越近权重越大
4. **质心坐标**：`cx = Σ(col × weight) / Σ(weight)`，`cy = Σ(row × weight) / Σ(weight)`
5. **有效判断**：至少 2 个区域检测到手且总权重 > 0.01 才认为存在手

### 速度计算

速度 = 当前帧质心 - 上一帧质心（单位：测区/帧）

```
velocity_x = current.cx - prev.cx
velocity_y = current.cy - prev.cy
speed = sqrt(velocity_x² + velocity_y²)
```

### 状态机

```
IDLE ──[手存在且 speed > 0.8]──→ TRACKING
IDLE ──[手存在且 speed < 0.3 连续10帧]──→ 触发 HOVER
IDLE ──[其他]──→ 重置确认计数

TRACKING ──[手消失]──→ 分类手势 → IDLE
TRACKING ──[speed > 0.8]──→ confirm_count++ → [≥3帧] → CONFIRMING
TRACKING ──[speed ≤ 0.8]──→ 误判 → IDLE

CONFIRMING ──[手消失或 speed < 0.4]──→ 分类手势 → IDLE
```

### 手势分类

```
speed = sqrt(vx² + vy²)
如果 speed < 0.8: → NONE

abs_vx = |vx|, abs_vy = |vy|
如果 abs_vx > abs_vy × 1.5: → 水平主导
    vx > 0 → SWIPE_RIGHT
    vx < 0 → SWIPE_LEFT
否则如果 abs_vy > abs_vx × 1.5: → 垂直主导
    vy > 0 → SWIPE_DOWN
    vy < 0 → SWIPE_UP
否则: → 对角线/模糊
    → PUSH（简化处理）
```

### 可调参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `VELOCITY_THRESHOLD` | 0.8 | 开始追踪的速度阈值（测区/帧） |
| `CONFIRM_FRAMES` | 3 | 确认手势需要的连续帧数 |
| `HOVER_VELOCITY` | 0.3 | 悬停判定的最大速度 |
| `HOVER_FRAMES` | 10 | 悬停确认需要的帧数 |
| `GESTURE_PRESENCE_DIST` | 1500 | 手存在检测的最大距离（mm） |
| `GESTURE_MIN_DIST` | 30 | 有效距离的最小值（mm） |

## VL53L5CX 平台移植

### 架构设计

VL53L5CX ULD 驱动通过 `VL53L5CX_Platform` 结构体中的函数指针实现平台抽象。本移植层扩展了该结构体，在标准字段之后追加 `i2c_ctx` 指针，用于区分两个传感器的 I2C 上下文。

```
VL53L5CX_Platform (标准字段)
    ├── address     ← 8位 I2C 地址 (0x52)
    ├── Write       ← 写函数指针
    ├── Read        ← 读函数指针
    └── GetTick     ← 毫秒时钟函数指针

vl53l5cx_hpm_platform_t (扩展)
    ├── base        ← VL53L5CX_Platform（必须放首位，保证兼容性）
    └── i2c_ctx     ← hpm_i2c_context_t* 指向对应 I2C 上下文
```

### 双传感器 I2C 路由

两个传感器共用相同的 `vl53l5cx_i2c_write` / `vl53l5cx_i2c_read` 函数，通过比较 `p_platform->address` 与 `hpm_plat_a.base.address` / `hpm_plat_b.base.address` 来确定使用哪个 I2C 上下文：

```
vl53l5cx_i2c_write(dev_addr, reg, data, size):
    addr7 = dev_addr >> 1          // 8位→7位地址转换
    如果 dev_addr == hpm_plat_a.base.address:
        ctx = &i2c_ctx_sensor_a    // I2C2
    否则如果 dev_addr == hpm_plat_b.base.address:
        ctx = &i2c_ctx_sensor_b    // I2C3
    hpm_i2c_master_addr_write_blocking(ctx, addr7, reg, 2, data, size)
```

### 平台函数实现

| 函数 | 实现方式 |
|------|----------|
| `RdByte` | 调用 `Read` 函数指针，读取 1 字节 |
| `WrByte` | 调用 `Write` 函数指针，写入 1 字节 |
| `RdMulti` | 调用 `Read` 函数指针，读取多字节 |
| `WrMulti` | 调用 `Write` 函数指针，写入多字节 |
| `SwapBuffer` | 4 字节一组做字节序交换（大端→小端） |
| `WaitMs` | 调用 `board_delay_ms()` |
| `GetTick` | 使用 `mchtmr` 计数器换算毫秒时间戳 |

### I2C 初始化参数

- **通信模式**：Master
- **时钟频率**：1MHz（VL53L5CX 支持最高 1MHz I2C）
- **寻址方式**：7-bit
- **从机地址**：0x29（7 位）/ 0x52（8 位）
- **寄存器地址**：16-bit

### 头文件桥接

VL53L5CX ULD 驱动的 `vl53l5cx_api.h` 包含 `"platform.h"`，但实际平台头文件名为 `vl53l5cx_platform.h`。为避免修改第三方驱动源码，创建了 `platform.h` 桥接头文件：

```c
/* platform.h - Bridge header for VL53L5CX ULD driver */
#include "vl53l5cx_platform.h"
```

## WS2812 驱动适配

### SPI 模式原理

WS2812 的数据协议要求每个 bit 的高电平占比不同来表示 0 和 1。SPI 模式下，通过调整发送的字节值来实现：

| WS2812 bit | SPI 发送字节 | 高电平占比 | 含义 |
|-----------|-------------|-----------|------|
| 0 | `0xE0` (0b11100000) | 3/8 ≈ 37.5% | 低电平时间较长 |
| 1 | `0xF8` (0b11111000) | 5/8 ≈ 62.5% | 高电平时间较长 |

SPI 时钟频率 = 8MHz → 1 个 SPI bit = 125ns → 1 个 WS2812 bit = 8 × 125ns = 1μs，符合 WS2812 时序规范。

### DMA 传输

WS2812 的 SPI 传输使用 HPM SDK 的 DMA 管理器，通过 `hpm_spi_dma_mgr_install_callback()` 注册 DMA 完成回调，`hpm_spi_transmit_nonblocking()` 启动非阻塞传输。需要在 CMakeLists.txt 中设置 `CONFIG_DMA_MGR=1` 来启用 DMA 管理器组件。

### 矩阵模式修复

原始 HPM_WS2812 驱动的 `WS2812_Update` 函数中，使用线性索引访问 3D 缓冲区 `WS2812_LED_Buffer[COL][ROW][24]`，当 LED 数量超过第一维大小时会导致越界。本示例修复了此问题：

```c
// 原始代码（矩阵模式下越界）:
buffer_type *buf = &WS2812_LED_Buffer[index][0];

// 修复后（根据连接模式选择索引方式）:
#if WS2812_LED_CONNECT == WS2812_CONNECT_LINE
    buf = &WS2812_LED_Buffer[index][0];
#elif WS2812_LED_CONNECT == WS2812_CONNECT_MATRIX
    buf = &WS2812_LED_Buffer[index / WS2812_LED_ROW][index % WS2812_LED_ROW][0];
#endif
```

### 板级配置 (ws2812_conf.h)

```c
#define WS2812_USE_SPI          1       // 使用 SPI 模式
#define WS2812_SPI              HPM_SPI1
#define WS2812_SPI_CLCOK        clock_spi1
#define WS2812_DIN              IOC_PAD_PA29
#define WS2812_LED_CONNECT      WS2812_CONNECT_MATRIX
#define WS2812_LED_COL          16
#define WS2812_LED_ROW          16
#define WS2812_LED_NUM          256     // 16 × 16
```

## 目录结构

```
samples/diy/vl53l5cx_ws2812/
├── CMakeLists.txt                          # 构建配置
├── README.md                               # 英文文档
├── README_zh.md                            # 中文文档
├── src/
│   ├── main.c                              # 主程序：串口菜单交互、模式切换
│   ├── gesture_engine.c                    # 手势识别引擎实现
│   ├── gesture_engine.h                    # 手势识别引擎接口
│   ├── app_depth_heatmap.c                 # 玩法1：深度热力图
│   ├── app_gesture_light.c                 # 玩法2：手势灯光跟随
│   └── app_ripple.c                        # 玩法3：涟漪效果
├── drivers/
│   ├── vl53l5cx/                           # VL53L5CX 驱动
│   │   ├── vl53l5cx_api.c                  # ST ULD 驱动主体
│   │   ├── vl53l5cx_api.h                  # ST ULD 驱动接口
│   │   ├── vl53l5cx_buffers.h              # 固件缓冲区数据
│   │   ├── vl53l5cx_platform.h             # 平台抽象定义
│   │   ├── vl53l5cx_platform_hpm.c         # HPM I2C 平台移植层 ★
│   │   ├── vl53l5cx_plugin_motion_indicator.c  # 运动指示器插件
│   │   ├── vl53l5cx_plugin_motion_indicator.h
│   │   └── platform.h                      # 桥接头文件 → vl53l5cx_platform.h
│   └── ws2812/                             # WS2812 驱动
│       ├── ws2812.c                         # 驱动主体（含矩阵模式修复）
│       ├── ws2812.h                         # 驱动接口
│       ├── ws2812_type.h                    # 数据类型定义
│       └── ws2812_conf.h                   # 板级配置 ★
```

标记 ★ 的文件为用户编写的板级适配代码，其余为第三方驱动源码。

## 第三方库

| 库 | 仓库地址 | 许可证 | 用途 |
|----|---------|--------|------|
| VL53L5CX ULD 驱动 | https://github.com/STMicroelectronics/stm32-vl53l5cx | BSD-3-Clause | VL53L5CX ToF 传感器驱动 |
| HPM_WS2812 驱动 | https://github.com/HalfSweet/HPM_WS2812 | Apache-2.0 | WS2812 LED 驱动（SPI/GPTMR+DMA） |

## 构建说明

### 前置条件

- HPM SDK 1.11.0+
- GNU RISC-V 工具链（支持 rv32imac_zicsr_zifencei 扩展）
- CMake 3.13+
- Ninja

### 编译步骤

```bash
# 设置环境变量
export GNURISCV_TOOLCHAIN_PATH=<工具链路径>
export HPM_SDK_BASE=<HPM SDK 路径>

# 配置
cmake -Bbuild -S. -GNinja -DBOARD=hpm5301evklite -DHPM_BUILD_TYPE=flash_xip

# 编译
ninja -C build
```

### 构建输出

编译生成的固件位于 `build/output/demo.elf`。

### 资源占用

| 区域 | 使用量 | 总量 | 占比 |
|------|--------|------|------|
| FLASH | 175 KB | 1 MB | 16.7% |
| ILM | 1.3 KB | 128 KB | 1.0% |
| DLM | 52 KB | 127 KB | 40.2% |

### 烧录

使用 OpenOCD 或 Segger J-Link 通过 cJTAG 接口烧录：

```bash
# OpenOCD 烧录示例
openocd -f board/hpm5301evklite.cfg -c "program build/output/demo.elf verify reset exit"
```

## CMakeLists.txt 关键配置

```cmake
# 启用 SPI/I2C 组件和 DMA 管理器
set(CONFIG_HPM_SPI 1)       # 包含 SPI 组件源码和头文件路径
set(CONFIG_HPM_I2C 1)       # 包含 I2C 组件源码和头文件路径
set(CONFIG_DMA_MGR 1)       # 启用 DMA 管理器（SPI DMA 传输需要）

# 链接数学库（sqrtf/fabsf）
sdk_ld_options("-lm")
```

**注意**：不能使用 `sdk_src()` 手动添加组件源码，否则组件 CMakeLists.txt 中的条件编译宏（如 `USE_DMA_MGR=1`）不会生效，导致链接时找不到 DMA 相关函数。

## API 接口

### 手势引擎

| 函数 | 说明 |
|------|------|
| `gesture_engine_init(engine, sensor_a, sensor_b)` | 初始化引擎，绑定两个 VL53L5CX 设备 |
| `gesture_engine_sensors_init(engine)` | 初始化双传感器（8×8 @ 15Hz），返回 0 成功 |
| `gesture_engine_capture_background(engine)` | 捕获背景参考帧 |
| `gesture_engine_process(engine)` | 处理一帧数据，返回是否检测到新手势 |
| `gesture_engine_get_last_gesture(engine)` | 获取最近手势事件 |
| `gesture_engine_get_centroid_a/b(engine)` | 获取传感器 A/B 当前质心 |
| `gesture_engine_get_distance_a/b(engine)` | 获取传感器 A/B 原始距离数据 |

### 应用模式

| 函数 | 说明 |
|------|------|
| `app_depth_heatmap_init/process` | 热力图模式初始化/每帧处理 |
| `app_gesture_light_init/process` | 灯光跟随模式初始化/每帧处理 |
| `app_ripple_init/process` | 涟漪效果模式初始化/每帧处理 |

### WS2812

| 函数 | 说明 |
|------|------|
| `WS2812_Init()` | 初始化 SPI 和 DMA |
| `WS2812_SetPixel(idx, r, g, b)` | 设置指定 LED 的 RGB 值 |
| `WS2812_MixPixel(idx, r, g, b)` | 混合指定 LED 的 RGB 值（取平均） |
| `WS2812_Update(blocking)` | 将缓冲区数据通过 SPI DMA 发送到 LED |
| `WS2812_IsBusy()` | 查询 DMA 传输是否完成 |

## 注意事项

1. **上电顺序**：先确保 VL53L5CX 传感器上电完成，再运行程序。传感器初始化时会检测 `is_alive`，失败会卡在错误提示。
2. **背景捕获**：首次启动会自动捕获背景。如果环境变化，按 `b` 重新捕获（捕获时确保没有手在传感器前方）。
3. **I2C 地址**：VL53L5CX 默认 8 位地址为 0x52，HPM SDK 的 I2C 组件使用 7 位地址 0x29，移植层已做转换。
4. **非缓存内存**：`VL53L5CX_Configuration` 结构体较大（约 2.3KB/个），包含嵌入式缓冲区，已使用 `ATTR_PLACE_AT_NONCACHEABLE_BSS` 放置在非缓存区，确保 I2C DMA 操作的一致性。
5. **蛇形布线**：LED 矩阵的物理布线为蛇形（S 形），代码中通过 `actual_col = (row % 2 == 0) ? col : (15 - col)` 处理奇偶行方向。
