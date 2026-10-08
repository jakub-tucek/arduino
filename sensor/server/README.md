# Local IoT stack

Podman Compose (also compatible with Docker Compose): ingestion API, SQLite,
Prometheus, and Grafana. No NAS dependency.

API payload, authentication, responses, and examples: [API.md](API.md).

## Deploy

Install `podman`, `podman-compose`, and `aardvark-dns`. Copy this directory to
`/opt/iot`. Copy `.env.example` to `.env`, set the public URL and strong random
secrets, and chmod `.env` to 0600. Never commit the populated file.

```sh
cd /opt/iot
sudo podman-compose up -d --build
sudo podman-compose logs
sudo podman stats --no-stream
```

Deployment currently uses rootful Podman. Container processes run as non-root
users; ports are bound only to localhost. Keep the Podman network private.
If UFW blocks container DNS, allow port 53 TCP/UDP from the compose subnet to
its bridge gateway, and permit forwarding only within that bridge/subnet.
Do not open service ports publicly.

A local `iot-stack.service` starts `podman-compose up -d` at boot and stops it
with `podman-compose stop`. Keep the populated Caddyfile and service-specific
credentials on the server, outside Git.

## Routes and auth

Caddy serves the homepage and proxies `/api/readings` unchanged. It strips
`/grafana` and `/prometheus` before forwarding to their backends. Grafana's root
URL includes `/grafana/`; Prometheus uses its external URL with route prefix `/`.
Monitoring paths require Caddy basic auth; Grafana additionally requires its
own login (`admin` and `GRAFANA_PASSWORD`).

POST JSON using the [firmware payload](../firmware/README.md#api-payload) with
`Authorization: Bearer <API_KEY>`. This initial implementation has one shared
key; per-device credentials and quotas are future improvements. Firmware must
be updated to send the header before uploads can work.

Public API paths are `/api/readings` and unauthenticated `GET /api/health`
(database-read readiness check). `/health` and `/metrics` stay internal. Prometheus scrapes the latest stored reading and its
receipt time every 30 seconds. Grafana includes a provisioned Sensors dashboard.

## Limits and persistence

- API: 128 MiB, 0.5 CPU, 64 processes/threads.
- Prometheus: 384 MiB, 0.5 CPU, 128 processes/threads.
- Grafana: 256 MiB, 0.5 CPU, 128 processes/threads.
- Prometheus retains up to 15 days / 1 GB (whichever limit applies first).
- Named volumes persist SQLite, Prometheus data, and Grafana state.

SQLite raw history currently has no automatic pruning: monitor volume usage and
plan backups/retention. Use SQLite's online backup API for consistent backups.
Do not run `down -v` unless you intend to delete stored data. Requests are limited
to 4 KiB with a 10-second socket timeout. Rate limiting and per-device quotas are
not yet implemented. Container images are version-pinned; review updates regularly.
