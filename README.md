# Industrial Sorting Machine Prototype

Portfolio documentation and source code for my master's thesis project: a PLC-controlled prototype of an industrial sorting machine using embedded modules, wired Modbus TCP communication and computer vision.

![Prototype sorting station](assets/real-station.jpg)

## What the machine does

The prototype moves one workpiece at a time on a conveyor, stops it under the camera and classifies it by QR code or by physical size. The PLC controls the sequence, sends tasks to the embedded modules and decides which receiving bin should be used. The machine also supports manual operation between automatic sequences, emergency stop priority, communication timeout handling and basic HMI diagnostics.

## System overview

![Communication diagram](assets/communication-diagram.png)

| Area | Implementation |
| --- | --- |
| Main controller | Siemens PLC logic developed in TIA Portal and tested with PLCSIM Advanced |
| HMI | Operator screens for auto/manual mode, alarms, counters and diagnostics |
| Sensor module | ESP32, W5500 Ethernet module and two VL53L0X distance sensors |
| Drive module | STM32F411, W5500 Ethernet module, two stepper drivers, conveyor and diverter motors |
| Vision module | Raspberry Pi, camera, Node-RED flow and a local OpenCV/Flask recognition service |
| Communication | Wired Ethernet, Modbus TCP, cyclic reads and task-based commands |

## Selected results

![State machine](assets/state-machine.png)

The prototype was verified through communication timing tests, positioning tests and classification tests. In the positioning test, the measured spread of the stopped workpiece position was 4.3 mm, which stayed below the theoretical limit estimated from communication delays and braking. The system was also tested for approximately 30 minutes with the selected communication timeout threshold without false timeout events.

![Positioning test](assets/positioning-test.jpg)

## Vision examples

| QR recognition | Size recognition |
| --- | --- |
| ![QR recognition](assets/qr-detection-known.jpg) | ![Size recognition](assets/size-detection-small.jpg) |
| ![Unknown QR result](assets/qr-detection-unknown.jpg) | ![Medium package size result](assets/size-detection-medium.jpg) |

## HMI and flow examples

| HMI auto mode | Node-RED communication flow |
| --- | --- |
| ![HMI automatic mode](assets/hmi-auto-mode.png) | ![Node-RED flow](assets/node-red-flow.png) |

## Repository structure

```text
.
|-- assets/                    # Selected thesis figures and prototype photos
|-- docs/                      # Short technical documentation in English
|-- firmware/
|   |-- esp32-sensor-module/    # ESP32 + VL53L0X + W5500 Modbus TCP server
|   `-- stm32-drive-module/     # STM32 stepper control and Modbus TCP task handler
`-- software/
    `-- vision-module/          # Raspberry Pi OpenCV/Flask recognition service
```

More details are available in:

- [Architecture](docs/architecture.md)
- [Communication model](docs/communication.md)
- [Testing summary](docs/testing.md)
- [Thesis summary](docs/thesis-summary.md)

## Notes

The repository is prepared as a portfolio version of the project. It includes the important application code and selected documentation assets, not the full thesis source tree or IDE build output.
