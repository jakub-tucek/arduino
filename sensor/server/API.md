# Sensor ingestion API

## Endpoint

`POST https://<iot-hostname>/api/readings`

HTTPS is required. Send one reading per sensor per request. No batch ingestion
or query API is currently implemented.

## Authentication

Required header:

```http
Authorization: Bearer <API_KEY>
Content-Type: application/json
```

The initial deployment uses a shared key, not separate per-device keys. Obtain
it from the server's private `/opt/iot/.env`; keep device copies in ignored
`config.h` files. Never commit keys or put them in URLs. Changing the server key
requires recreating the API container and updating all devices.

## Payload

```json
{
  "device_id": "AA:BB:CC:DD:EE:FF",
  "device_name": "living-room",
  "sensor_id": "room",
  "sensor_type": "bmp280",
  "temperature": 22.4,
  "pressure_hpa": 1013.25,
  "timestamp": 30123
}
```

| Required field | Meaning and validation |
|----------------|------------------------|
| `device_id` | Stable device identifier, 1–64 characters |
| `sensor_id` | Stable identifier within the device, 1–64 characters |
| `temperature` | Finite numeric value in °C, between -80 and 150 inclusive |
| `pressure_hpa` | Finite numeric value in hPa, between 100 and 1200 inclusive |
| `timestamp` | Device uptime in milliseconds, between 0 and 4294967295 inclusive |

Identifiers allow ASCII letters, digits, spaces, and `-_: .`. Temperature and
pressure are converted to floats, timestamp to an integer; numeric strings are
currently accepted. Optional `device_name` and `sensor_type` fields, as well as
other additional fields, are currently ignored and not stored. BMP280 does not
measure humidity.

The server records its own receipt time for history. Device uptime is not a
wall-clock timestamp and resets on reboot. Request body limit: 4096 bytes. Send
`Content-Length`; chunked request bodies are not supported by the ingestion app.

## Responses

| Status | Body / meaning |
|--------|----------------|
| `201` | `{"status":"stored"}` — SQLite insert committed |
| `400` | `{"error":"invalid payload"}` — missing or invalid reading fields |
| `401` | `{"error":"unauthorized"}` — missing or incorrect bearer key |
| `404` | `{}` — unknown API path |
| `413` | `{}` — empty body or body over 4096 bytes |
| `502` | Proxy cannot reach the ingestion service |

Only POST is supported for ingestion. A GET to `/api/readings` returns 404.
Unsupported HTTP methods may return 501 from the underlying HTTP server.
Malformed `Content-Length` returns 400. There is no idempotency key or
deduplication: retrying a successful upload stores another row. Failures/timeouts
may leave the client uncertain whether a reading was committed.

## Example

Use a private environment variable rather than embedding a key in this document:

```sh
curl --fail-with-body "https://<iot-hostname>/api/readings" \
  -H "Authorization: Bearer ${API_KEY}" \
  -H 'Content-Type: application/json' \
  --data '{"device_id":"example-device","sensor_id":"room","temperature":22.4,"pressure_hpa":1013.25,"timestamp":30123}'
```

ESP32 firmware must add the bearer header to its HTTP client and validate the
server certificate using a trusted root CA. Never disable TLS verification.

## Health check

`GET https://<iot-hostname>/api/health` requires no API key and checks that the
SQLite database can be opened and the readings table queried.

- Healthy: `200 {"status":"ok"}`.
- Database unavailable: `503 {"status":"unavailable"}`.

```sh
curl --fail https://<iot-hostname>/api/health
```

This is a lightweight database-read check, not a guarantee of disk space or
write availability. Responses expose no credentials or device data.

## Internal endpoints

Not routed publicly through Caddy:

- `GET /health` — same check as `/api/health`.
- `GET /metrics` → Prometheus text exposition of the latest reading per
  `(device_id, sensor_id)`:
  - `sensor_temperature_celsius`
  - `sensor_pressure_hpa`
  - `sensor_last_received_seconds` (Unix server receipt time)

These internal endpoints have no application authentication: rely on localhost
port binding and the private container network. Prometheus scrapes every 30
seconds. SQLite stores each accepted reading, while Prometheus samples latest
values, so it does not necessarily retain every individual uploaded reading.

## Current limitations

Shared credential, no per-device authorization, rate limits, quotas, automatic
SQLite retention, or persistent firmware upload queue. Rotate compromised keys
and monitor disk usage. Keep SQLite backups separate from the live volume.
