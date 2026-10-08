# External-server gateway

Planned project. Runs on the external server.

Responsibilities:

- Accept ESP32 reading POSTs at a public HTTPS endpoint.
- Terminate TLS with Caddy or nginx.
- Forward the reading path to the NAS API through Tailscale.
- Pass device authentication to the ingestion API.

Deployment config still needs the public domain, NAS Tailscale address,
API port/path, and server runtime. A reverse proxy alone does not queue
readings when the NAS is offline.

See [system layout](../README.md) and [firmware payload](../firmware/README.md#api-payload).
