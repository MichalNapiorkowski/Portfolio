# STM32 Drive Module

STM32F411 firmware for the conveyor and diverter drive module. The STM32 receives Modbus TCP tasks from the PLC and generates step pulses for two stepper motors with hardware timers.

## Hardware

- STM32F411 board,
- W5500 Ethernet module over SPI,
- two stepper motor drivers controlled by STEP/DIR/ENABLE signals,
- Hall sensor for the diverter home position.

## Main operations

- start and stop the conveyor,
- move the diverter to one of three bin positions,
- return the diverter to the home position,
- report task result, motion state and homing state through Modbus registers.

## Task examples

| Task ID | Operation |
| --- | --- |
| `0x0001` | automatic conveyor run |
| `0x0002` | conveyor stop |
| `0x0003` | emergency stop |
| `0x000A` | automatic divert to bin 1 |
| `0x000B` | automatic divert to bin 2 |
| `0x000C` | automatic divert to bin 3 |
| `0x0010` | manual conveyor run |
| `0x0020` | manual conveyor reverse |
| `0xF000` | diverter homing |

## Build note

This folder contains the application code, `.ioc` configuration, startup/linker files and the W5500/Modbus libraries used by the firmware. STM32 HAL/CMSIS vendor files and build output are not included. They can be regenerated from `MODBUS.ioc` in STM32CubeIDE.

The watchdog function is present in the source, but its call in the main loop is left as it was in the tested project.
