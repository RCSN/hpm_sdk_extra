# SPI单线双线四线操作nor flash存储器

## 概述

- 该实例工程展示了使用SPI接口读写nor flash存储器。
- 该实例的nor flash存储器使用的W25Q64JVSSIQ
- 组件serial_nor支持遵守sfdp的nor flash存储器，不限定W25Q64JVSSIQ
- 默认SPI SCLK频率为50M
- 默认SPI的IO模式为双线SPI

## 硬件设置
- [SPI引脚](lab_board_app_spi_pin)根据板子型号查看具体信息
- SPI引脚对应好nor flash(模块)引脚
- 可参考淘宝链接模块: https://item.taobao.com/item.htm?spm=a1z10.3-c-s.w4002-24145583300.20.7e233746j65hgo&id=43071752309
- 使用四线的时候，需要确保板载有引出SPI的IO2，IO3引脚并做好相关引脚初始化。

## 运行现象

- 确认硬件无误后，工程正确运行后

- 使用四线SPI时，串口终端会输出如下信息：
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

- 使用两线SPI时，串口终端会输出如下信息：
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

- 使用单线SPI时，串口终端会输出如下信息：
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