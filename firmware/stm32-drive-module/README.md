# STM32 Drive Module

STM32F411 firmware for the conveyor and diverter drive module. The STM32 receives Modbus TCP tasks from the PLC and generates deterministic step pulses for two stepper motors using hardware timers.

## Hardware

- STM32F411 board,
- W5500 Ethernet module connected over SPI,
- two stepper motor drivers controlled by STEP/DIR/ENABLE signals,
- Hall sensor used for the diverter home position.

## Main responsibilities

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

## Repository note

This folder keeps the application code, the `.ioc` configuration, startup/linker files and the lightweight W5500/Modbus libraries used by the application. Generated STM32 HAL/CMSIS vendor files and build output are intentionally not included. They can be regenerated from `MODBUS.ioc` in STM32CubeIDE.

The current source contains a communication watchdog function. Its call in the main loop is left in the same state as in the working project, so the portfolio version does not change runtime behavior.
