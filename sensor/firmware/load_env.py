"""Load local deployment values without putting secrets in compiler arguments."""
import json
import math
from pathlib import Path

Import("env")

project = Path(env.subst("$PROJECT_DIR"))
values = {}
dotenv = project / env.GetProjectOption("custom_env_file", ".env")
if not dotenv.exists():
    raise ValueError("Missing local environment profile: " + dotenv.name)
if dotenv.exists():
    for line in dotenv.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        key, separator, value = line.partition("=")
        if not separator:
            raise ValueError("Invalid .env entry; expected KEY=value")
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key.strip()] = value

names = ("API_KEY", "API_ENDPOINT", "WIFI_SSID", "WIFI_PASSWORD", "DEVICE_NAME", "SENSOR_ID",
         "LOCATION", "GROUP", "WIFI_HOSTNAME")
lines = ["#pragma once", "// Generated from ignored local environment profile; do not commit."]
for name in names:
    if name in values:
        lines.append("#define SENSOR_{} {}".format(name, json.dumps(values[name])))
if "TEMPERATURE_OFFSET_C" in values:
    offset = float(values["TEMPERATURE_OFFSET_C"])
    if not math.isfinite(offset):
        raise ValueError("TEMPERATURE_OFFSET_C must be finite")
    lines.append("#define SENSOR_TEMPERATURE_OFFSET_C " + json.dumps(offset) + "F")
if values.get("API_ROOT_CA_FILE"):
    pem = (project / values["API_ROOT_CA_FILE"]).read_text()
    lines.append("#define SENSOR_API_ROOT_CA " + json.dumps(pem))

build = Path(env.subst("$BUILD_DIR"))
build.mkdir(parents=True, exist_ok=True)
header = build / "device_secrets.h"
content = "\n".join(lines) + "\n"
if not header.exists() or header.read_text() != content:
    header.write_text(content)
header.chmod(0o600)
env.Append(CPPPATH=[str(build)])
