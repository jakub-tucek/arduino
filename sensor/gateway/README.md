# IoT public front door

Runs on the public server. Caddy terminates HTTPS for the IoT hostname and routes
paths to local services on the same server.

Current direction: skip the NAS hop. The server hosts the ingestion API,
monitoring stack, and dashboard directly.

## Public paths

- `https://iot.<domain>/` — landing page with links.
- `https://iot.<domain>/api/*` — dedicated ingestion API for ESP32 devices.
- `https://iot.<domain>/grafana/` — Grafana UI, protected by auth.
- `https://iot.<domain>/prometheus/` — Prometheus UI, protected by auth/admin-only.

## Routing

```text
ESP32 → https://iot.<domain>/api/readings
      → Caddy on public server
      → local ingestion API on 127.0.0.1:8095
      → SQLite / metrics on the same server
```

Caddy should only expose HTTPS entrypoints. Backends should listen on localhost
or a private container network.

## Security contract

- API authentication belongs in the ingestion app, e.g. `Authorization: Bearer …`
  or `X-API-Key` per device.
- Caddy may reject obviously invalid requests, but it is not the source of truth
  for device authentication.
- Grafana and Prometheus must require auth if exposed publicly.
- Prometheus, node exporters, databases, and internal metrics endpoints should
  not listen on public interfaces.
- Do not commit real domains, API keys, Grafana passwords, or local Caddyfiles.

## Files in this repo

- `Caddyfile.example` — public template only, safe to commit.
- `.gitignore` — prevents committing the real `Caddyfile`, `.env`, or local
  deployment overrides.

See [system layout](../README.md) and [firmware payload](../firmware/README.md#api-payload).
