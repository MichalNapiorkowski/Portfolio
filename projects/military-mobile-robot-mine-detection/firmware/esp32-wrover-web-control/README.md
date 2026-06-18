# ESP32 WROVER Web Control

Firmware for the robot operator interface. The ESP32 WROVER module runs a web server, displays the camera stream and exposes controls for movement, speed, detector sensitivity and automatic stop.

Main file: `camera_web_control_server.ino`

Included files:

| File | Purpose |
| --- | --- |
| `camera_web_control_server.ino` | Wi-Fi setup, camera server, HTTP handlers and UART command routing |
| `html_page.h` | Browser control panel served by the ESP32 |
| `camera_pins.h` | Camera pin mapping for the WROVER camera module |
| `camera_index.h` | Camera web-server support file |
| `partitions.csv` | ESP32 partition layout used by the sketch |

Wi-Fi credentials are intentionally replaced with placeholders in the portfolio copy.
