# ESP32 Sensor Module

ESP32 firmware for detecting workpiece presence at two conveyor positions. The module exposes sensor states and error bits through a Modbus TCP server over W5500 Ethernet.

## Hardware

- ESP32 WROOM development board,
- W5500 Ethernet module over SPI,
- two VL53L0X time-of-flight distance sensors over I2C,
- separate XSHUT lines for assigning individual sensor addresses.

## Register data

| Register | Meaning |
| --- | --- |
| `1` | detection bits for sensor 1 and sensor 2 |
| `2` | initialization and runtime error bits |
| `3-10` | reserved |

## Behavior

The firmware initializes each VL53L0X separately, reads both sensors in the main loop and updates Modbus holding registers. Detection uses hysteresis near the threshold. If a sensor stops responding, the firmware recovers the I2C bus and tries to initialize the sensor again.

## Source file

The main sketch is `esp32_sensor_module.ino`.
