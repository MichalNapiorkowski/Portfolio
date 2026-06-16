# Thesis Summary

## Topic

Development and implementation of a prototype industrial sorting machine using a PLC controller and embedded systems.

## Goal

The goal was to build and verify a working prototype that connects PLC logic with distributed embedded modules responsible for sensing, motion control and vision-based recognition.

## Scope

The work covered:

- mechanical concept and 3D printed prototype elements,
- PLC control logic and HMI screens,
- ESP32 distance sensor module,
- STM32 stepper motor drive module,
- Raspberry Pi vision module,
- wired Modbus TCP communication,
- tests of communication timing, positioning and recognition quality.

## Practical result

The final prototype performs an automatic sorting cycle. It detects a workpiece, stops it under the camera, recognizes the QR code or size, moves the diverter to the selected position and sends the workpiece to the target bin.
