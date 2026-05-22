# VL53L5CX + WS2812 Gesture Interactive Demo

## Overview

This project implements a gesture-interactive demo on the hpm5301evklite board using two VL53L5CX ToF sensors and a 16×16 WS2812 LED matrix.

The VL53L5CX is an 8×8 multi-zone Time-of-Flight ranging sensor from STMicroelectronics. A single sensor provides 64 independent ranging zones. This demo uses two VL53L5CX sensors on separate I2C buses (I2C2 and I2C3), eliminating the need to modify sensor I2C addresses. Together they provide 128 ranging zones covering a hand-sized interaction space.

The 16×16 WS2812 LED matrix is driven via the SPI1 MOSI signal line, using the HPM SDK's SPI component with DMA transfer. Each ToF ranging zone maps to a 2×2 LED pixel block, enabling real-time distance visualization.

The demo provides three interactive modes: depth heatmap, gesture light following, and ripple effect, along with a built-in gesture recognition engine supporting left/right/up/down swipe, push/pull, and hover detection.

## Hardware Connections

### Target Board

- **Board**: hpm5301evklite
- **MCU**: HPM5301 (RISC-V, rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs)

### Pin Assignment

| Peripheral | Pin | Function | Description |
|---|---|---|---|
| I2C2 | PB08 | SCL | VL53L5CX Sensor A clock line |
| I2C2 | PB09 | SDA | VL53L5CX Sensor A data line |
| I2C3 | PB13 | SCL | VL53L5CX Sensor B clock line |
| I2C3 | PB12 | SDA | VL53L5CX Sensor B data line |
| SPI1 | PA29 | MOSI | WS2812 DIN data input |

### Connection Notes

- **No address change needed**: The two VL53L5CX sensors are on separate I2C buses, both using the default 7-bit address 0x29 (8-bit address 0x52). No LPn pin control is required to modify addresses.
- **WS2812 requires MOSI only**: In SPI mode, only one MOSI signal line is needed. The SPI clock runs at 8MHz, encoding each WS2812 data bit with 3 SPI bits (`0xE0` for 0, `0xF8` for 1).
- **Snake-pattern wiring**: The 16×16 matrix uses serpentine wiring — even rows go left-to-right, odd rows go right-to-left. This is handled in the code.

## Demo Modes

Switch modes by entering characters via the serial terminal (115200 8N1):

| Key | Mode | Description |
|---|---|---|
| `1` | Depth Heatmap | Distance → HSV color band mapping, near=warm/far=cool |
| `2` | Gesture Light Following | Hand centroid → LED cursor, swipe changes color |
| `3` | Ripple Effect | Hand appearance triggers 2D ripple animation |
| `b` | Capture Background | Record current frame as background reference (no hand present) |
| `r` | Reset Sensors | Stop ranging and re-initialize sensors |

### Mode 1: Depth Heatmap

Maps the 8×8 distance data from both VL53L5CX sensors onto the 16×16 LED matrix:

- **Sensor A** → left half of the matrix (columns 0-7)
- **Sensor B** → right half of the matrix (columns 8-15)
- Each ToF zone → 2×2 LED pixels

Distance-to-color mapping logic:
- Distance range: 30mm ~ 1500mm
- Near (30mm) → hue 200 (warm: red/orange/yellow), intuitively representing "hot"
- Far (1500mm) → hue 0 (cool: blue/violet), intuitively representing "cold"
- Out of range → black (LED off)
- Very far (> 1200mm) → brightness gradually fades

The HSV → RGB conversion uses pure integer arithmetic with no floating-point overhead, suitable for real-time MCU operation.

### Mode 2: Gesture Light Following

Tracks the hand centroid in real-time and displays a following cursor on the LED matrix:

- **Cursor structure**:
  - Center (radius < 2): full brightness 255, saturation 255
  - Glow (radius 2-5): brightness linearly attenuates, saturation 200
  - Trail effect: each frame mixes existing pixels with black via `MixPixel(0,0,0)` for gradual fade

- **Color mapping**:
  - Base hue = distance offset (near=warm, far=cool)
  - Swipe left → hue -30
  - Swipe right → hue +30
  - Repeated swiping traverses the entire hue wheel

- **Coordinate mapping**: centroid coordinates (0.0~7.0) × 2 → LED coordinates (0.0~15.0)

### Mode 3: Ripple Effect

When a hand enters the detection area, a 2D ripple animation is triggered at the hand position:

- **Ripple parameters**:
  - Up to 4 simultaneous ripples (circular buffer)
  - Maximum radius 12 (auto-destroyed after exceeding)
  - Expansion speed: 0.3 per frame
  - Ring thickness: 2.0 pixels
  - Each new ripple increments hue by 40, creating a rainbow color sequence

- **Drawing logic**:
  - Calculate distance from each LED to the ripple center: `dist`
  - Ring distance: `ring_dist = |dist - radius|`
  - When `ring_dist < RIPPLE_WIDTH`, the pixel is on the ring
  - Intensity = `1 - ring_dist / RIPPLE_WIDTH` (brightest at ring center, fading at edges)
  - Brightness decays as radius expands: `brightness × (1 - radius / max_radius)`
  - Multiple ripples use `MixPixel` for additive blending

- **Hand position indicator**: when a hand is present, a gray dot (80,80,80) is displayed at the corresponding centroid position

## Gesture Recognition Engine

The gesture engine (`gesture_engine.c/h`) implements centroid tracking + velocity state machine for gesture detection.

### Core Data Structures

```
centroid_t           - Per-frame centroid: cx/cy position, avg_distance, present flag, area count
sensor_tracker_t     - Sensor tracker: trail buffer, previous centroid, state machine state,
                       confirm count, velocity vector
gesture_event_t      - Gesture event: gesture type, speed, centroid position, average distance
gesture_engine_t     - Engine context: dual sensor configs, dual ranging results,
                       dual trackers, background reference
```

### Centroid Computation

For each sensor's 8×8=64 zones:

1. **Filtering**: skip invalid readings (distance ≤ 30mm or ≥ 1500mm)
2. **Background subtraction**: if background is captured, skip zones with < 100mm difference from background (excludes static objects)
3. **Weighted centroid**: weight = `(1500 - distance) / 1500`, closer zones get higher weight
4. **Centroid coordinates**: `cx = Σ(col × weight) / Σ(weight)`, `cy = Σ(row × weight) / Σ(weight)`
5. **Validity check**: at least 2 zones must detect a hand and total weight > 0.01

### Velocity Computation

Velocity = current frame centroid - previous frame centroid (unit: zones/frame)

```
velocity_x = current.cx - prev.cx
velocity_y = current.cy - prev.cy
speed = sqrt(velocity_x² + velocity_y²)
```

### State Machine

```
IDLE ──[hand present AND speed > 0.8]──→ TRACKING
IDLE ──[hand present AND speed < 0.3 for 10 consecutive frames]──→ HOVER detected
IDLE ──[other]──→ reset confirm count

TRACKING ──[hand disappeared]──→ classify gesture → IDLE
TRACKING ──[speed > 0.8]──→ confirm_count++ → [≥3 frames] → CONFIRMING
TRACKING ──[speed ≤ 0.8]──→ false alarm → IDLE

CONFIRMING ──[hand disappeared OR speed < 0.4]──→ classify gesture → IDLE
```

### Gesture Classification

```
speed = sqrt(vx² + vy²)
If speed < 0.8: → NONE

abs_vx = |vx|, abs_vy = |vy|
If abs_vx > abs_vy × 1.5: → horizontal dominant
    vx > 0 → SWIPE_RIGHT
    vx < 0 → SWIPE_LEFT
Else if abs_vy > abs_vx × 1.5: → vertical dominant
    vy > 0 → SWIPE_DOWN
    vy < 0 → SWIPE_UP
Else: → diagonal/ambiguous
    → PUSH (simplified)
```

### Tunable Parameters

| Parameter | Default | Description |
|---|---|---|
| `VELOCITY_THRESHOLD` | 0.8 | Speed threshold to start tracking (zones/frame) |
| `CONFIRM_FRAMES` | 3 | Consecutive frames required to confirm gesture |
| `HOVER_VELOCITY` | 0.3 | Maximum speed for hover detection |
| `HOVER_FRAMES` | 10 | Frames required to confirm hover |
| `GESTURE_PRESENCE_DIST` | 1500 | Maximum distance for hand presence (mm) |
| `GESTURE_MIN_DIST` | 30 | Minimum valid distance (mm) |

## VL53L5CX Platform Port

### Architecture

The VL53L5CX ULD driver uses function pointers in the `VL53L5CX_Platform` struct for platform abstraction. This port extends the struct by appending an `i2c_ctx` pointer after the standard fields, used to distinguish I2C contexts for the two sensors.

```
VL53L5CX_Platform (standard fields)
    ├── address     ← 8-bit I2C address (0x52)
    ├── Write       ← write function pointer
    ├── Read        ← read function pointer
    └── GetTick     ← millisecond tick function pointer

vl53l5cx_hpm_platform_t (extended)
    ├── base        ← VL53L5CX_Platform (must be first for compatibility)
    └── i2c_ctx     ← hpm_i2c_context_t* pointing to the corresponding I2C context
```

### Dual-Sensor I2C Routing

Both sensors share the same `vl53l5cx_i2c_write` / `vl53l5cx_i2c_read` functions. The correct I2C context is determined by comparing `p_platform->address` with `hpm_plat_a.base.address` / `hpm_plat_b.base.address`:

```
vl53l5cx_i2c_write(dev_addr, reg, data, size):
    addr7 = dev_addr >> 1          // 8-bit → 7-bit address conversion
    If dev_addr == hpm_plat_a.base.address:
        ctx = &i2c_ctx_sensor_a    // I2C2
    Else if dev_addr == hpm_plat_b.base.address:
        ctx = &i2c_ctx_sensor_b    // I2C3
    hpm_i2c_master_addr_write_blocking(ctx, addr7, reg, 2, data, size)
```

### Platform Function Implementations

| Function | Implementation |
|---|---|
| `RdByte` | Calls `Read` function pointer, reads 1 byte |
| `WrByte` | Calls `Write` function pointer, writes 1 byte |
| `RdMulti` | Calls `Read` function pointer, reads multiple bytes |
| `WrMulti` | Calls `Write` function pointer, writes multiple bytes |
| `SwapBuffer` | Swaps byte order in 4-byte groups (big-endian → little-endian) |
| `WaitMs` | Calls `board_delay_ms()` |
| `GetTick` | Uses `mchtmr` counter converted to millisecond timestamp |

### I2C Initialization Parameters

- **Communication mode**: Master
- **Clock frequency**: 1MHz (VL53L5CX supports up to 1MHz I2C)
- **Addressing mode**: 7-bit
- **Slave address**: 0x29 (7-bit) / 0x52 (8-bit)
- **Register address**: 16-bit

### Header Bridge

The VL53L5CX ULD driver's `vl53l5cx_api.h` includes `"platform.h"`, but the actual platform header is named `vl53l5cx_platform.h`. To avoid modifying third-party driver source, a `platform.h` bridge header was created:

```c
/* platform.h - Bridge header for VL53L5CX ULD driver */
#include "vl53l5cx_platform.h"
```

## WS2812 Driver Adaptation

### SPI Mode Principle

The WS2812 data protocol requires different high-level duty cycles to represent 0 and 1. In SPI mode, this is achieved by adjusting the transmitted byte values:

| WS2812 bit | SPI byte | High duty | Meaning |
|---|---|---|---|
| 0 | `0xE0` (0b11100000) | 3/8 ≈ 37.5% | Longer low period |
| 1 | `0xF8` (0b11111000) | 5/8 ≈ 62.5% | Longer high period |

SPI clock = 8MHz → 1 SPI bit = 125ns → 1 WS2812 bit = 8 × 125ns = 1μs, conforming to the WS2812 timing specification.

### DMA Transfer

The WS2812 SPI transfer uses the HPM SDK's DMA manager. `hpm_spi_dma_mgr_install_callback()` registers the DMA completion callback, and `hpm_spi_transmit_nonblocking()` starts a non-blocking transfer. The CMakeLists.txt must set `CONFIG_DMA_MGR=1` to enable the DMA manager component.

### Matrix Mode Fix

The original HPM_WS2812 driver's `WS2812_Update` function uses a linear index to access the 3D buffer `WS2812_LED_Buffer[COL][ROW][24]`, causing out-of-bounds access when the LED count exceeds the first dimension size. This demo fixes the issue:

```c
// Original code (out-of-bounds in matrix mode):
buffer_type *buf = &WS2812_LED_Buffer[index][0];

// Fixed (select indexing method based on connection mode):
#if WS2812_LED_CONNECT == WS2812_CONNECT_LINE
    buf = &WS2812_LED_Buffer[index][0];
#elif WS2812_LED_CONNECT == WS2812_CONNECT_MATRIX
    buf = &WS2812_LED_Buffer[index / WS2812_LED_ROW][index % WS2812_LED_ROW][0];
#endif
```

### Board Configuration (ws2812_conf.h)

```c
#define WS2812_USE_SPI          1       // Use SPI mode
#define WS2812_SPI              HPM_SPI1
#define WS2812_SPI_CLCOK        clock_spi1
#define WS2812_DIN              IOC_PAD_PA29
#define WS2812_LED_CONNECT      WS2812_CONNECT_MATRIX
#define WS2812_LED_COL          16
#define WS2812_LED_ROW          16
#define WS2812_LED_NUM          256     // 16 × 16
```

## Directory Structure

```
samples/diy/vl53l5cx_ws2812/
├── CMakeLists.txt                          # Build configuration
├── README.md                               # English documentation
├── README_zh.md                            # Chinese documentation
├── src/
│   ├── main.c                              # Main program: serial menu, mode switching
│   ├── gesture_engine.c                    # Gesture recognition engine implementation
│   ├── gesture_engine.h                    # Gesture recognition engine interface
│   ├── app_depth_heatmap.c                 # Mode 1: depth heatmap
│   ├── app_gesture_light.c                 # Mode 2: gesture light following
│   └── app_ripple.c                        # Mode 3: ripple effect
├── drivers/
│   ├── vl53l5cx/                           # VL53L5CX driver
│   │   ├── vl53l5cx_api.c                  # ST ULD driver core
│   │   ├── vl53l5cx_api.h                  # ST ULD driver interface
│   │   ├── vl53l5cx_buffers.h              # Firmware buffer data
│   │   ├── vl53l5cx_platform.h             # Platform abstraction definitions
│   │   ├── vl53l5cx_platform_hpm.c         # HPM I2C platform port ★
│   │   ├── vl53l5cx_plugin_motion_indicator.c  # Motion indicator plugin
│   │   ├── vl53l5cx_plugin_motion_indicator.h
│   │   └── platform.h                      # Bridge header → vl53l5cx_platform.h
│   └── ws2812/                             # WS2812 driver
│       ├── ws2812.c                         # Driver core (with matrix mode fix)
│       ├── ws2812.h                         # Driver interface
│       ├── ws2812_type.h                    # Data type definitions
│       └── ws2812_conf.h                   # Board configuration ★
```

Files marked with ★ are user-written board-level adaptation code; the rest are third-party driver source.

## Third-Party Libraries

| Library | Repository | License | Purpose |
|---|---|---|---|
| VL53L5CX ULD Driver | https://github.com/STMicroelectronics/stm32-vl53l5cx | BSD-3-Clause | VL53L5CX ToF sensor driver |
| HPM_WS2812 Driver | https://github.com/HalfSweet/HPM_WS2812 | Apache-2.0 | WS2812 LED driver (SPI/GPTMR+DMA) |

## Build Instructions

### Prerequisites

- HPM SDK 1.11.0+
- GNU RISC-V toolchain (with rv32imac_zicsr_zifencei extension support)
- CMake 3.13+
- Ninja

### Build Steps

```bash
# Set environment variables
export GNURISCV_TOOLCHAIN_PATH=<toolchain_path>
export HPM_SDK_BASE=<hpm_sdk_path>

# Configure
cmake -Bbuild -S. -GNinja -DBOARD=hpm5301evklite -DHPM_BUILD_TYPE=flash_xip

# Build
ninja -C build
```

### Build Output

The compiled firmware is located at `build/output/demo.elf`.

### Resource Usage

| Region | Used | Total | Usage |
|---|---|---|---|
| FLASH | 175 KB | 1 MB | 16.7% |
| ILM | 1.3 KB | 128 KB | 1.0% |
| DLM | 52 KB | 127 KB | 40.2% |

### Flashing

Use OpenOCD or Segger J-Link via the cJTAG interface:

```bash
# OpenOCD flash example
openocd -f board/hpm5301evklite.cfg -c "program build/output/demo.elf verify reset exit"
```

## CMakeLists.txt Key Configuration

```cmake
# Enable SPI/I2C components and DMA manager
set(CONFIG_HPM_SPI 1)       # Includes SPI component sources and header paths
set(CONFIG_HPM_I2C 1)       # Includes I2C component sources and header paths
set(CONFIG_DMA_MGR 1)       # Enables DMA manager (required for SPI DMA transfer)

# Link math library (sqrtf/fabsf)
sdk_ld_options("-lm")
```

**Important**: Do not use `sdk_src()` to manually add component source files. This bypasses the component CMakeLists.txt, preventing conditional compilation macros (such as `USE_DMA_MGR=1`) from taking effect, resulting in undefined reference errors for DMA-related functions at link time.

## API Reference

### Gesture Engine

| Function | Description |
|---|---|
| `gesture_engine_init(engine, sensor_a, sensor_b)` | Initialize engine, bind two VL53L5CX devices |
| `gesture_engine_sensors_init(engine)` | Initialize dual sensors (8×8 @ 15Hz), returns 0 on success |
| `gesture_engine_capture_background(engine)` | Capture background reference frame |
| `gesture_engine_process(engine)` | Process one frame, returns true if new gesture detected |
| `gesture_engine_get_last_gesture(engine)` | Get the most recent gesture event |
| `gesture_engine_get_centroid_a/b(engine)` | Get current centroid for sensor A/B |
| `gesture_engine_get_distance_a/b(engine)` | Get raw distance data for sensor A/B |

### Application Modes

| Function | Description |
|---|---|
| `app_depth_heatmap_init/process` | Heatmap mode init/per-frame processing |
| `app_gesture_light_init/process` | Light following mode init/per-frame processing |
| `app_ripple_init/process` | Ripple effect mode init/per-frame processing |

### WS2812

| Function | Description |
|---|---|
| `WS2812_Init()` | Initialize SPI and DMA |
| `WS2812_SetPixel(idx, r, g, b)` | Set RGB value for specified LED |
| `WS2812_MixPixel(idx, r, g, b)` | Blend RGB value for specified LED (averages with existing) |
| `WS2812_Update(blocking)` | Send buffer data to LEDs via SPI DMA |
| `WS2812_IsBusy()` | Query whether DMA transfer is complete |

## Notes

1. **Power-on sequence**: Ensure VL53L5CX sensors are powered up before running the program. Sensor initialization checks `is_alive` and will halt with an error message on failure.
2. **Background capture**: An initial background is captured automatically at startup. If the environment changes, press `b` to recapture (ensure no hand is in front of the sensors during capture).
3. **I2C address**: VL53L5CX default 8-bit address is 0x52. The HPM SDK's I2C component uses 7-bit address 0x29. The port layer handles the conversion.
4. **Non-cacheable memory**: The `VL53L5CX_Configuration` struct is large (~2.3KB each) with embedded buffers. Both instances are placed in non-cacheable BSS using `ATTR_PLACE_AT_NONCACHEABLE_BSS` to ensure I2C DMA coherency.
5. **Snake-pattern wiring**: The LED matrix uses serpentine (S-pattern) physical wiring. The code handles odd/even row direction via `actual_col = (row % 2 == 0) ? col : (15 - col)`.
