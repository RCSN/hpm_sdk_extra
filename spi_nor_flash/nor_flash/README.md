# SPI single/dual/quad opration nor flash

## Overview

- The example shows the use of SPI interface to read and write nor flash memory.
- The nor flash memory used in this example is W25Q64JVSSIQ
- The component serial_nor supports nor flash memory that complies with sfdp, and is not limited to W25Q64JVSSIQ
- The default SPI SCLK frequency is 50M
- The default SPI IO mode is dual-wire SPI

## Board Setting

- [SPI PINs](lab_board_app_spi_pin) Check the information according to the board model
- The SPI pins connect the nor flash(module) pins
- please refer to taobao link module: https://item.taobao.com/item.htm?spm=a1z10.3-c-s.w4002-24145583300.20.7e233746j65hgo&id=43071752309
- When using four wires, you need to ensure that the board has IO2 and IO3 pins leading to SPI and initialize the relevant pins.


## Running the example

- confirm the hardware is correct,and the sample runs successfully,

- Use quad SPI, the log would be seen on the terminal like:
```console
spi nor flash init ok
the flash sfdp version:5
the flash size:8192 KB
the flash page_size:256 Byte
the flash sector_size:4 KB
the flash block_size:64 KB
the flash sector_erase_cmd:0x20
the flash block_erase_cmd:0xd8
wbuff and rbuff compare finsh ok
write_speed:667.72 KB/s read_speed:22842.98 KB/s
page program: wbuff and rbuff compare finsh ok
```

- Use dual SPI, the log would be seen on the terminal like:
```console
spi nor flash init ok
the flash sfdp version:0
the flash size:8192 KB
the flash page_size:256 Byte
the flash sector_size:4 KB
the flash block_size:64 KB
the flash sector_erase_cmd:0x20
the flash block_erase_cmd:0xd8
wbuff and rbuff compare finsh ok
write_speed:617.63 KB/s read_speed:11905.44 KB/s
page program: wbuff and rbuff compare finsh ok
```

- Use single SPI, the log would be seen on the terminal like:
```console
spi nor flash init ok
the flash sfdp version:0
the flash size:8192 KB
the flash page_size:256 Byte
the flash sector_size:4 KB
the flash block_size:64 KB
the flash sector_erase_cmd:0x20
the flash block_erase_cmd:0xd8
wbuff and rbuff compare finsh ok
write_speed:614.73 KB/s read_speed:6084.88 KB/s
page program: wbuff and rbuff compare finsh ok
```
