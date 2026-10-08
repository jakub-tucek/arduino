import hmac
import json
import math
import os
import sqlite3
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

KEY = os.environ['API_KEY']
if len(KEY) < 32:
    raise RuntimeError('API_KEY must contain at least 32 characters')
DB = os.environ.get('DATABASE', '/data/readings.sqlite')
lock = threading.Lock()
with sqlite3.connect(DB) as db:
    db.execute('PRAGMA journal_mode=WAL')
    db.execute('CREATE TABLE IF NOT EXISTS readings (received_at REAL, device_id TEXT, sensor_id TEXT, temperature REAL, pressure_hpa REAL, uptime INTEGER)')
    db.execute('CREATE INDEX IF NOT EXISTS history ON readings(device_id,sensor_id,received_at)')


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body, content_type='application/json'):
        data = body.encode()
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path in ('/health', '/api/health'):
            try:
                with sqlite3.connect(f'file:{DB}?mode=rw', uri=True, timeout=2) as db:
                    db.execute('SELECT received_at FROM readings LIMIT 1').fetchone()
            except sqlite3.Error:
                return self.reply(503, '{"status":"unavailable"}')
            return self.reply(200, '{"status":"ok"}')
        if self.path != '/metrics':
            return self.reply(404, '{}')
        with lock, sqlite3.connect(DB) as db:
            rows = db.execute('SELECT device_id,sensor_id,temperature,pressure_hpa,received_at FROM readings WHERE rowid IN (SELECT MAX(rowid) FROM readings GROUP BY device_id,sensor_id)').fetchall()
        lines = []
        for device, sensor, temp, pressure, received in rows:
            labels = 'device_id=' + json.dumps(device) + ',sensor_id=' + json.dumps(sensor)
            for name, value in [('temperature_celsius', temp), ('pressure_hpa', pressure), ('last_received_seconds', received)]:
                lines.append(f'sensor_{name}{{{labels}}} {value}')
        self.reply(200, '\n'.join(lines) + '\n', 'text/plain; version=0.0.4')

    def do_POST(self):
        if self.path != '/api/readings':
            return self.reply(404, '{}')
        auth = self.headers.get('Authorization', '')
        if not hmac.compare_digest(auth.encode(), ('Bearer ' + KEY).encode()):
            return self.reply(401, '{"error":"unauthorized"}')
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if not 0 < length <= 4096:
                return self.reply(413, '{}')
            data = json.loads(self.rfile.read(length))
            device, sensor = data['device_id'], data['sensor_id']
            for value in (device, sensor):
                if not isinstance(value, str) or not 1 <= len(value) <= 64 or not all(c.isascii() and (c.isalnum() or c in '-_: .') for c in value):
                    raise ValueError('invalid identifier')
            temp, pressure = float(data['temperature']), float(data['pressure_hpa'])
            uptime = int(data['timestamp'])
            if not math.isfinite(temp) or not -80 <= temp <= 150 or not math.isfinite(pressure) or not 100 <= pressure <= 1200 or not 0 <= uptime <= 4294967295:
                raise ValueError('invalid measurement')
        except (ValueError, KeyError, TypeError, UnicodeDecodeError):
            return self.reply(400, '{"error":"invalid payload"}')
        with lock, sqlite3.connect(DB) as db:
            db.execute('INSERT INTO readings VALUES (?,?,?,?,?,?)', (time.time(), device, sensor, temp, pressure, uptime))
        self.reply(201, '{"status":"stored"}')

    def log_message(self, format, *args):
        pass  # Do not log authorization headers or payloads.


class Server(ThreadingHTTPServer):
    daemon_threads = True
    def get_request(self):
        sock, address = super().get_request()
        sock.settimeout(10)
        return sock, address


Server(('0.0.0.0', 8095), Handler).serve_forever()
