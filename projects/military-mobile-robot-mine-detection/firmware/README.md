# Firmware

Selected firmware from the engineering thesis prototype.

| Folder | Main file | Description |
| --- | --- | --- |
| `esp32-wrover-web-control` | `camera_web_control_server.ino` | Web interface, camera stream, Wi-Fi status and UART command routing |
| `esp32-wroom-motor-controller` | `stepper_motor_controller.ino` | Stepper motor control, motion profiles and TMC2209 setup |
| `pulse-induction-metal-detector` | `src/dma_capture.c` | PlatformIO/Pico SDK detector firmware with ADC/DMA sampling and UART output |
| `sampling-test` | `adc_sampling_test.ino` | Measurement helper used while checking ADC sampling behavior |

Each folder contains its own README with the local role and included files.
