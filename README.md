# Common

This project demonstrates the capabilities of USB. 

Applicable for stm32f1xx series processors.

The project does not require libraries other the standard ones for stm32f1xx.

# Features

The project implements USB CDC: USB-COM converter.

Only the source code is provided (c and h files).

For logical structuring the source code for working with the hardware used is encapsulated in separate files.

## Led

Frequency is 1 Hz. 

You can exclude led sources from project.

## Usart

Only lines _TX_(PA2) and _RX_(PA3) of _USART2_ are used.

The settings can be changed by standard USB CDC requests.

_USART2_ _DMA_ channels for receiving and transmitting are used.

## USB

Only USB FS 2.0 is used.

_Windows 10_ loads the standart driver automaticaly. 

Manufacturer: FG
Product: FG Virtual ComPort
VID: 0x03EB
PID: 0x6119
