# External-server gateway

Runs on the external server. This component is only a reverse proxy: accept the
public HTTPS request and forward it to the NAS API through Tailscale.

It must not authenticate, validate, transform, store, or queue readings. The NAS
owns ingestion logic and storage.

## Implementation decision

Use **Caddy** on the external server:

- automatic Let's Encrypt HTTPS
- small static config
- no application runtime to maintain
- proxy target can be the NAS Tailscale DNS name or Tailscale IP

The ESP32 posts to the public gateway URL. Caddy forwards the same request path
to the NAS API over Tailscale.

```text
ESP32 → https://<sensor-public-domain>/api/readings
      → Caddy on external server
      → http://<nas-tailnet-host>:<nas-api-port>/api/readings
```

The sensor hostname exposes only the NAS API under `/api`. Keep the external and
NAS paths identical so the gateway remains a plain proxy.

## Files in this repo

- `Caddyfile.example` — public template only, safe to commit.
- `.gitignore` — prevents committing the real `Caddyfile`, `.env`, or local
  deployment overrides.

Do not commit the real public domain, NAS address, tailnet name, private paths,
or any credentials here. Keep actual deployment config on the server.

## Setup checklist

On the external server:

1. Install and authenticate Tailscale.
2. Verify the NAS is reachable over Tailscale:
   ```sh
   tailscale status
   curl http://<nas-tailnet-host>:<nas-api-port>/api/health
   ```
3. Install Caddy.
4. Copy `Caddyfile.example` to `/etc/caddy/Caddyfile` on the server.
5. Replace placeholders with the real public hostname and NAS API target.
6. Reload Caddy:
   ```sh
   sudo caddy validate --config /etc/caddy/Caddyfile
   sudo systemctl reload caddy
   ```
7. Test through the public endpoint:
   ```sh
   curl -i https://<sensor-public-domain>/api/health
   ```

## Proxy contract

- Public path and NAS path should match, e.g. `/api/readings`.
- Gateway should return NAS responses as-is where possible.
- If the NAS is offline, uploads fail; the gateway does not queue readings.
- Authentication headers/body are forwarded to the NAS unchanged.

See [system layout](../README.md) and [firmware payload](../firmware/README.md#api-payload).
