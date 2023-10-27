# Nor flash Msc disk

## Overview

- The example shows the use of SPI interface to read and write nor flash memory.
- The nor flash memory used in this example is W25Q64JVSSIQ
- The component serial_nor supports nor flash memory that complies with sfdp, and is not limited to W25Q64JVSSIQ
- The default SPI SCLK frequency is 50M
- The default SPI IO mode is dual-wire SPI
- Use cherryusb protocol stack to use nor flash as U disk

## Board Setting

- [SPI PINs](lab_board_app_spi_pin) Check the information according to the board model
- The SPI pins connect the nor flash(module) pins
- please refer to taobao link module: https://item.taobao.com/item.htm?spm=a1z10.3-c-s.w4002-24145583300.20.7e233746j65hgo&id=43071752309
- When using four wires, you need to ensure that the board has IO2 and IO3 pins leading to SPI and initialize the relevant pins.
- Connect a USB port on PC to the PWR DEBUG port on the development board with a USB Type-C cable
- Connect a USB port on PC to one of USB port on the development board with a USB Type-C cable

## Running the example

- Download the program and run. The computer can automatically recognize and enumerate a USB disk device.
- Double click to open the USB disk. Format the USB disk according to the computer prompts to get the USB disk of about 500KB in size
- You can copy a file to the USB disk, and then copy it from the USB disk.
