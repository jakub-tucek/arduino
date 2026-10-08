# Wi-Fi environmental sensors

`sensor/firmware/` is the shared firmware for temperature sensor nodes.
Each ESP32 reads one or
two GY-BMP280 modules and POSTs one JSON reading per sensor to the same API.

## Hardware

Each Wi-Fi node needs an ESP32 and a BMP280. The BMP280 itself has no Wi-Fi.
BMP280 measures temperature and pressure only; it has no humidity sensor.

| GY-BMP280 | ESP32 default |
|-----------|---------------|
| VCC / VIN | 3V3 |
| GND | GND |
| SDA / SDI | GPIO 21 |
| SCL / SCK | GPIO 22 |
| CSB / CS | GPIO 25 (firmware drives HIGH) |
| SDO | GPIO 26 (firmware drives LOW for `0x76`) |

Use 3.3 V power and logic. On modules with exposed CS and SDO pins, CS must
be high for I²C mode; SDO low selects `0x76`, high selects `0x77`.
Check your module's jumpers/pin labels. Boards with fixed addresses may need
a solder jumper change to use the second address.

The default config drives CSB and SDO from GPIOs so one sensor needs no shared
3V3 connection. GPIOs are high-impedance during reset; hardware pull-ups/pull-downs
give defined levels before firmware starts. Do not also connect these driven
pins directly to power rails. For hardware-set mode/address, use `-1` for the
corresponding GPIO in `SENSORS`, for example `{"room", 0x76, -1, -1}`.

The second example sensor uses CSB on GPIO 27 and SDO on GPIO 33; firmware
drives its SDO HIGH for `0x77`. VCC/GND/SDA/SCL remain shared between sensors.

Two modules can share SDA/SCL if their addresses differ. More than two on
one ESP32 need an I²C multiplexer or another bus and a firmware extension.
For sensors in different rooms, use separate ESP32 Wi-Fi nodes.

## Setup

Run from `sensor/firmware/`:

```sh
cp config.example.h config.h
# Edit config.h: Wi-Fi credentials, API endpoint, device name, sensor addresses.
pio run
pio run --target upload
pio device monitor -b 115200 -f direct
```

`config.h` is ignored by Git. Existing DHT22 configs need to be updated from
the new example; this firmware now uses BMP280 via I²C.

For multiple nodes:

- Flash the same firmware with each node's own `config.h`.
- Set `DEVICE_NAME` to a readable location, such as `living-room`.
- Leave `DEVICE_ID` empty to use the ESP32 MAC address, or set a unique stable ID.
- Give each entry in `SENSORS` a stable ID unique within that node.
- Store readings by `(device_id, sensor_id)` in the receiving API.

Enable the commented `0x77` entry in `SENSORS` for a second module on one node.
Each sensor initializes and uploads independently. Missing sensors retry on
the next cycle. Wi-Fi reconnects automatically; offline readings are logged
to serial and dropped. Failed uploads are dropped too, with no persistent queue.
The default cycle sleeps 30 seconds after reads/uploads; HTTP time adds to it.
The BMP280 uses forced mode: one measurement per cycle, then sensor sleep to
reduce self-heating. Temperature is chip temperature; compare against a
reference thermometer after both have settled in the same location.

## API payload

One POST per sensor, `Content-Type: application/json`:

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

Temperature is °C, pressure is hPa (absolute, not corrected to sea level).
The payload omits `humidity` because BMP280 cannot measure it.
`timestamp` is ESP32 uptime in milliseconds,
resets on reboot, and wraps after about 49.7 days. Use API receipt time for
wall-clock storage. The API must accept the added fields and return a 2xx
status on success.

For HTTPS, set `API_ROOT_CA` to the endpoint's trusted root CA PEM. The ESP32
syncs its clock through `NTP_SERVER`; HTTPS uploads wait for a valid clock.

## Tailscale

For this system, set `API_ENDPOINT` to the external gateway's public HTTPS
URL and configure `API_ROOT_CA`. The external server forwards to the NAS
through Tailscale; the ESP32 needs only normal Wi-Fi internet access.
See the [system layout](../README.md), [gateway](../gateway/), and [NAS](../nas/).
The LAN gateway options below are reference setups for other deployments.

ESP32 Arduino has no official Tailscale client. Use a Raspberry Pi, NAS, or
always-on computer on the sensor LAN as a Tailscale gateway.

### Sending readings to an API in your tailnet

The simplest setup is a LAN-facing HTTP reverse proxy on the gateway:

```text
BMP280 → ESP32 → Wi-Fi → gateway LAN address → Tailscale → API
```

1. Install Tailscale on the gateway and connect it to your tailnet.
2. Run a reverse proxy listening on its LAN address, forwarding the reading
   path to your API's Tailscale address/name. Allow that gateway to reach the
   API port in the tailnet access policy.
3. Set `API_ENDPOINT` to the gateway's LAN URL, for example
   `http://192.168.1.10:8080/readings`. Reserve the gateway's LAN IP in DHCP.
4. Verify the proxy POST from another LAN device, then flash the ESP32.

Tailscale encrypts the gateway-to-API hop. LAN HTTP remains plaintext; use
HTTPS and `API_ROOT_CA` if you need encryption on that hop too. Proxy deployment
is separate from this firmware.

### Reaching the ESP32 LAN from your tailnet

A [Tailscale subnet router](https://tailscale.com/kb/1019/subnets) lets tailnet
devices reach the ESP32's LAN IP without installing Tailscale on it. On a
Linux gateway with Tailscale installed and connected:

```sh
# Persist IP forwarding on the gateway.
printf 'net.ipv4.ip_forward = 1\nnet.ipv6.conf.all.forwarding = 1\n' | sudo tee /etc/sysctl.d/99-tailscale.conf
sudo sysctl -p /etc/sysctl.d/99-tailscale.conf
# Replace with your actual sensor LAN subnet.
sudo tailscale set --advertise-routes=192.168.1.0/24
```

Approve the advertised route in the Tailscale admin console and allow the
needed traffic in your tailnet policy. Linux clients also need
`sudo tailscale set --accept-routes`.

This firmware only sends readings; it exposes no HTTP server or OTA service.
Subnet routing gives network reachability, not a new management endpoint.

Advertising the LAN alone does **not** route ESP32-initiated requests to
`100.x` tailnet addresses. For direct outbound routing, configure a route
on your LAN router through the gateway, IP forwarding, and appropriate
NAT/return routing and tailnet policy. MagicDNS also needs a LAN DNS setup.
The LAN reverse proxy avoids those routing requirements. See Tailscale's
[site-to-site routing guide](https://tailscale.com/kb/1214/site-to-site) for
the routed approach.
