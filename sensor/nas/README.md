# NAS ingestion and storage

Planned project. Runs on the NAS alongside Tailscale.

Responsibilities:

- Accept readings forwarded by the external-server gateway.
- Authenticate devices and validate reading payloads.
- Identify sensors by `(device_id, sensor_id)`.
- Store readings in SQLite on a persistent local NAS volume.
- Record server receipt time; firmware `timestamp` is device uptime.

## Storage

Use SQLite with the ingestion API as its only writer. Keep the database file
on a local NAS volume, not an SMB/NFS mount. Enable WAL mode so reads can run
alongside ingestion.

Store `device_id`, `sensor_id`, server `received_at`, device uptime, temperature
in °C and pressure in hPa. BMP280 has no humidity measurement. Index
`(device_id, sensor_id, received_at)` for sensor history queries. Use SQLite's
online backup API for consistent backups while ingestion runs.

Implementation still needs a NAS runtime and API framework.
Limit tailnet access to the ingestion port to the gateway.

See [system layout](../README.md) and [firmware payload](../firmware/README.md#api-payload).
