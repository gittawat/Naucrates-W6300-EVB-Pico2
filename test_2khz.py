#!/usr/bin/env python3
"""2 kHz UDP echo stress test (64-byte payload) for W6300-EVB-Pico2."""

import socket
import time
import struct

HOST = "10.10.10.10"
PORT = 27181
RATE = 2000
DURATION = 5
INTERVAL = 1.0 / RATE
PAYLOAD_SIZE = 64

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.connect((HOST, PORT))
sock.settimeout(INTERVAL * 3)

sent = 0
rtts = []

payload = bytearray(PAYLOAD_SIZE)
start = time.monotonic()
deadline = start + DURATION
next_send = start

print(f"Sending {PAYLOAD_SIZE}-byte packets at {RATE} Hz for {DURATION}s to {HOST}:{PORT}...")

while time.monotonic() < deadline:
    now = time.monotonic()
    if now >= next_send:
        payload[:12] = struct.pack('<IQ', sent, int(now * 1_000_000))
        sock.send(payload)
        sent += 1
        next_send += INTERVAL

    try:
        data = sock.recv(PAYLOAD_SIZE)
        seq, send_us = struct.unpack('<IQ', data[:12])
        rtts.append(time.monotonic() * 1_000_000 - send_us)
    except socket.timeout:
        pass

time.sleep(0.5)
while True:
    try:
        data = sock.recv(PAYLOAD_SIZE)
        seq, send_us = struct.unpack('<IQ', data[:12])
        rtts.append(time.monotonic() * 1_000_000 - send_us)
    except socket.timeout:
        break

sock.close()

lost = sent - len(rtts)
loss_pct = lost / sent * 100 if sent else 0
elapsed = time.monotonic() - start

print(f"\n--- Results ({elapsed:.1f}s) ---")
print(f"Payload:    {PAYLOAD_SIZE} bytes")
print(f"Sent:       {sent}")
print(f"Echoed:     {len(rtts)}")
print(f"Lost:       {lost} ({loss_pct:.1f}%)")
if rtts:
    rtts.sort()
    print(f"Latency min/avg/max: {min(rtts):.0f} / {sum(rtts)/len(rtts):.0f} / {max(rtts):.0f} us")
    print(f"P50/P99:    {rtts[len(rtts)//2]:.0f} / {rtts[int(len(rtts)*0.99)]:.0f} us")
    print(f"Throughput: {len(rtts)/elapsed:.0f} Hz")
