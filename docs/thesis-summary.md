# Thesis Summary

## Topic

Development and implementation of a prototype industrial sorting machine using a PLC controller and embedded systems.

## Goal

Build a working prototype that connects PLC sequence logic with embedded modules for sensing, motion control and vision recognition.

## Scope

The work included:

- mechanical concept and 3D printed prototype parts,
- PLC control logic and HMI screens,
- ESP32 distance sensor module,
- STM32 stepper motor drive module,
- Raspberry Pi vision module,
- wired Modbus TCP communication,
- tests of communication timing, positioning and recognition quality.

## Result

The prototype performs an automatic sorting cycle: it detects a workpiece, stops it under the camera, recognizes a QR code or size, moves the diverter and sends the workpiece to the selected bin.
