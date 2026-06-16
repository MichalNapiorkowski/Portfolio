# Engineering Portfolio

Selected engineering projects from my studies and thesis work. The repository is organized as a small portfolio, so each project has its own README, selected code, figures and short technical notes.

## Projects

| Project | Preview | Main topics |
| --- | --- | --- |
| [Industrial Sorting Machine Prototype](projects/industrial-sorting-machine-prototype/) | <img src="projects/industrial-sorting-machine-prototype/assets/real-station.jpg" width="260" alt="Sorting machine prototype"> | PLC control, PLCSIM Advanced, HMI, Modbus TCP, ESP32, STM32, Raspberry Pi vision, OpenCV |
| [Military Mobile Robot for Mine Detection](projects/military-mobile-robot-mine-detection/) | <img src="projects/military-mobile-robot-mine-detection/assets/robot-isometric.jpg" width="260" alt="Mine detection robot prototype"> | ESP32 web control, camera stream, TMC2209 stepper control, pulse-induction metal detector, 3D-printed chassis |

## Web control example

The mine-detection robot is controlled from a browser panel served directly by the ESP32 WROVER module. The panel combines the camera preview, driving buttons, speed setting, detector sensitivity and basic status indicators.

<p align="center">
  <img src="projects/military-mobile-robot-mine-detection/assets/web-control-interface.png" width="560" alt="ESP32 web control interface">
</p>

## Repository note

The repository contains selected, readable project files prepared for portfolio review. Temporary IDE files, generated build output, old experimental folders and full thesis source trees are not included.
