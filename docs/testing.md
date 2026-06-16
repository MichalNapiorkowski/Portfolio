# Testing Summary

## Communication timing

Communication times were measured for the drive, sensor and vision modules. Raspberry Pi was also checked over Wi-Fi because that interface was assigned by default at the start. The wired setup was kept for the final tests because the response times were repeatable.

## Positioning test

![Positioning test](../assets/positioning-test.jpg)

The positioning test used the vision module to mark the workpiece center relative to the camera field. The measured spread was 4.3 mm. The estimated maximum displacement from communication delay and braking was 5.87 mm.

## Recognition test

The recognition test covered known QR codes, an unknown QR code and three size classes. Repeated detection attempts were kept in the cycle because one QR case failed on the first attempt and was recognized correctly on the next one.

## Error handling

The prototype was checked by disconnecting modules during operation. The PLC detected missing responses through timeout handling and switched the sequence to an error state.
