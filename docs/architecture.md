# Architecture

## Mechanical layout

![CAD model](../assets/cad-model.png)

The station is built around a short conveyor and a three-position diverter. The CAD model was used to define the camera position, detection area and receiving bin locations before printing the mechanical parts.

![Prototype modules](../assets/electronics-modules.jpg)

The electronics are intentionally kept as a prototype setup instead of a final control cabinet. Each module has its own local electronics and communicates with the PLC over wired Ethernet.

## Module responsibilities

| Module | Main role | Hardware |
| --- | --- | --- |
| PLC | Sequence control, mode handling, alarms, task dispatching | PLCSIM Advanced during development |
| ESP32 sensor module | Workpiece presence detection and sensor error reporting | ESP32, W5500, 2x VL53L0X |
| STM32 drive module | Conveyor motion, diverter motion and homing state | STM32F411, W5500, 2x stepper drivers, Hall sensor |
| Raspberry Pi vision module | QR and size recognition | Raspberry Pi, camera, OpenCV, Node-RED |

## Control concept

The PLC remains the main decision point. Embedded modules execute local hardware-related tasks and return compact results through Modbus registers. This keeps timing-critical motor pulses on the STM32 timer while leaving the machine sequence in the PLC program.
