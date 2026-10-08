# Sensor system

Three projects live here:

| Project | Role | Status |
|---------|------|--------|
| [firmware](firmware/) | ESP32 + GY-BMP280 nodes send readings over Wi-Fi | Live temperature/pressure readings verified; uploads pending |
| [gateway](gateway/) | External server accepts HTTPS and forwards through Tailscale | Planned |
| [nas](nas/) | NAS ingestion API and SQLite store readings | Planned |

```text
BMP280 → ESP32 → Wi-Fi → external server HTTPS gateway
                                      ↓ Tailscale
                               NAS API → SQLite
```

The ESP32 uses the gateway's public HTTPS URL. Tailscale runs on the external
server and NAS. Data lives in SQLite on a local NAS volume. No gateway on the
sensor LAN is needed.

Firmware setup and wiring: [firmware/README.md](firmware/README.md).

Before deploying, add per-device authentication to the firmware and ingestion
path. Current firmware drops readings when uploads fail; retaining readings
through NAS outages needs a queue.
