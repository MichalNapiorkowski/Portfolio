# ESP32 WROOM Motor Controller

Firmware for the motor-control module. The ESP32 WROOM receives short UART commands from the web-control module and controls the stepper motor drivers.

Main file: `stepper_motor_controller.ino`

The code handles speed/current profiles, direction commands and TMC2209 driver configuration. It is kept close to the thesis version so the communication between the web panel and the drive module remains visible.
