# nor flash存储器模拟U盘

## 概述

- 该实例工程展示了使用SPI接口读写nor flash存储器。
- 该实例的nor flash存储器使用的W25Q64JVSSIQ
- 组件serial_nor支持遵守sfdp的nor flash存储器，不限定W25Q64JVSSIQ
- 默认SPI SCLK频率为50M
- 默认SPI的IO模式为双线SPI
- 使用cherryusb协议栈对nor flash存储器模拟成U盘

## 硬件设置
- [SPI引脚](lab_board_app_spi_pin)根据板子型号查看具体信息
- SPI引脚对应好nor flash(模块)引脚
- 可参考淘宝链接模块: https://item.taobao.com/item.htm?spm=a1z10.3-c-s.w4002-24145583300.20.7e233746j65hgo&id=43071752309
- 使用四线的时候，需要确保板载有引出SPI的IO2，IO3引脚并做好相关引脚初始化。
- 使用USB Type-C线缆连接PC USB端口和PWR DEBUG端口
- 使用USB Type-C线缆连接PC USB端口和开发板USB0端口

## 运行现象

- 将程序下载至开发板运行，电脑可自动识别并枚举出一个U盘
- 双击打开U盘，根据电脑提示将U盘格式化，得到一个跟nor flash存储区容量大小的U盘
- 可以将文件copy至U盘，然后从U盘copy出来，可当做U盘使用
