# Arduino Transit Stop Board

ESP32 Arduino projects collection for IoT and display applications.

## Projects

| Project | Description |
|---------|-------------|
| **[transit-stop-board](transit-stop-board/)** | 240x320 touch LCD showing live transit departures via a departureboards API |
| **[sensor](sensor/)** | Sensor system: ESP32 GY-BMP280 firmware, external-server gateway, and NAS ingestion/storage |

## Getting Started

The transit board and `sensor/firmware/` are standalone PlatformIO projects.
The sensor gateway and NAS projects have their own deployment setup. See each
project's README.

### Prerequisites

- [PlatformIO](https://platformio.org/)
- ESP32-DevKitC board (ESP32-WROOM-32D)

### Common Setup

```bash
# Copy config example and edit with your settings
cp config.example.h config.h

# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor -b 115200 -f direct
```
