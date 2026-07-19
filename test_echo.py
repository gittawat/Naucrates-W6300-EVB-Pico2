#!/usr/bin/env python3
"""UDP echo client for W6300-EVB-Pico2 test."""

import socket
import time

HOST = "10.10.10.10"
PORT = 27181

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Bind local side so we can receive the echo
sock.bind(("", 0))
sock.settimeout(2.0)

messages = [b"hello w6300!", b"ping", b"echo test", b"0123456789" * 5]

for msg in messages:
    sock.sendto(msg, (HOST, PORT))
    print(f"sent {len(msg)} bytes: {msg!r}")

    try:
        data, addr = sock.recvfrom(1024)
        print(f"echo  {len(data)} bytes from {addr}: {data!r}")
        if data == msg:
            print("  PASS\n")
        else:
            print("  MISMATCH\n")
    except socket.timeout:
        print("  TIMEOUT - no response\n")
        break

sock.close()
