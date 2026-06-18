# Pulse-Induction Metal Detector

PlatformIO/Pico SDK version of the detector firmware based on the `adc_dma_capture` project. The code samples the analog detector response with the RP2040 ADC and DMA, calculates averaged sample values and sends the detection state over UART.

Main file: `src/dma_capture.c`

| File | Purpose |
| --- | --- |
| `src/dma_capture.c` | ADC/DMA capture, threshold calculation, detection decision and UART output |
| `src/resistor_dac.pio` | PIO helper program kept with the original Pico SDK source set |

The original build output and local VS Code/Pico SDK folders are not included.
