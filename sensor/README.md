# Sensor system

| Project | Role | Status |
|---------|------|--------|
| [firmware](firmware/) | ESP32 + BMP280 readings over Wi-Fi | Upload authentication header still needed |
| [gateway](gateway/) | Caddy HTTPS homepage and protected monitoring routes | Deployed |
| [server](server/) | Local API, SQLite, Prometheus, Grafana via Podman Compose | Deployed |
| [nas](nas/) | Previous NAS storage proposal | Superseded by local server stack |

```text
BMP280 → ESP32 → HTTPS /api/readings → Caddy → local ingestion → SQLite
                                                   ↓ /metrics
                                              Prometheus → Grafana
```

The ESP32 POSTs to the gateway's public HTTPS URL, such as
`https://iot.example.com/api/readings`. Set the real endpoint in the ignored
`firmware/config.h`.

The public server hosts the complete stack. No NAS dependency or sensor-LAN
gateway is needed. Backend ports bind only to localhost. Caddy exposes the
homepage, authenticated `/grafana/` and `/prometheus/`, and the API endpoint.
Real deployment configuration and secrets stay outside Git.

Firmware setup: [firmware/README.md](firmware/README.md).
Server deployment and limits: [server/README.md](server/README.md).

The API currently uses a shared bearer key. Firmware must send it; per-device
keys remain future work. Failed uploads are dropped rather than queued.
