# Testing Summary

## Communication timing

Communication times were measured for the drive, sensor and vision modules. The vision module was also checked over Wi-Fi for comparison, because Raspberry Pi initially used that interface by default. The wired configuration was kept for the final prototype because it gave repeatable operation and matched industrial practice better.

## Positioning test

![Positioning test](../assets/positioning-test.jpg)

The positioning test used the vision module to mark the workpiece center relative to the camera field. The measured position spread was 4.3 mm. The estimated maximum displacement caused by communication delay and braking was 5.87 mm, so the measured spread stayed inside the expected range.

## Recognition test

The recognition test covered known QR codes, an unknown QR code and three size classes. Repeated detection attempts were useful because a single failed recognition did not always mean that the workpiece was placed incorrectly. In one QR case, a repeated attempt led to a correct final recognition.

## Error handling

The prototype was also checked by disconnecting modules during operation. The PLC detected missing module responses through timeout handling and moved the machine logic into an error state.
