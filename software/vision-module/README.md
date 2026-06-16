# Vision Module

Raspberry Pi service used by the sorting machine to recognize QR codes and workpiece size. The PLC does not call this service directly. Node-RED handles Modbus registers and calls the local HTTP endpoints when a new vision task arrives.

## Endpoints

| Endpoint | Result |
| --- | --- |
| `/read_qr` | QR string, `UNRECOGNIZED` or `ERROR` |
| `/check_size` | workpiece size as `longer_mm,shorter_mm`, `UNRECOGNIZED` or `ERROR` |
| `/check_center_offset` | diagnostic image saved on disk, empty HTTP response |

## Processing

QR recognition uses OpenCV `QRCodeDetector` with several prepared image variants. Size recognition uses HSV masking, contour filtering and rectangle measurement. The service saves the processed region of interest after each detection, which helps with diagnostics during tests.

## Run on Raspberry Pi

```bash
python3 qr_camera_server.py
```

The service listens on `127.0.0.1:5000`, so it is intended to be used locally by Node-RED.
