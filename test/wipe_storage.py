#!/usr/bin/env python3
"""
JoyCore-FW Storage Wipe Utility

Purpose:
  Completely remove all stored configuration and version files from the device
  so that on next boot the firmware treats it as a first-time startup and
  generates fresh defaults (only when config is absent).

Mechanism:
  Uses the existing serial command 'FORMAT_STORAGE' which formats the EEPROM
  file table. This is preferable to issuing individual deletions because it
  guarantees a clean table and frees all space.

Workflow:
  1. Detect the serial port (heuristic similar to test scripts).
  2. Open the port at 115200 baud.
  3. Send 'FORMAT_STORAGE' followed by newline.
  4. Read and display response lines for a short timeout.
  5. Optionally send 'LIST_FILES' to confirm it's empty.

Safety:
  This will irreversibly clear all stored config on the board. Make sure you
  really intend to wipe before running.

Prereqs:
  pip install pyserial

Usage:
  python wipe_storage.py
"""
import time
import sys
from typing import Optional

TRY_PORT_HINTS = ["pico", "arduino", "usb serial", "rp2040"]


def find_device_port() -> Optional[str]:
    try:
        import serial.tools.list_ports
    except ImportError:
        print("pyserial not installed. Install with: pip install pyserial")
        return None
    ports = serial.tools.list_ports.comports()
    for port in ports:
        desc = (port.description or "").lower()
        if any(h in desc for h in TRY_PORT_HINTS):
            return port.device
    # fallback: return first port if any
    return ports[0].device if ports else None


def send_command(ser, cmd: str, wait: float = 0.2) -> str:
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode('utf-8'))
    ser.flush()
    time.sleep(wait)
    out = []
    start = time.time()
    while time.time() - start < 2.0:  # up to 2 seconds
        if ser.in_waiting:
            try:
                out.append(ser.read(ser.in_waiting).decode('utf-8', errors='ignore'))
            except Exception:
                break
        else:
            time.sleep(0.05)
    return ''.join(out)


def main() -> int:
    try:
        import serial  # type: ignore
    except ImportError:
        print("pyserial not installed. Install with: pip install pyserial")
        return 1

    port = find_device_port()
    if not port:
        print("Could not auto-detect JoyCore device serial port.")
        return 1
    print(f"Using serial port: {port}")

    try:
        ser = serial.Serial(port, 115200, timeout=0.2)
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        return 1

    # Allow board to reset / enumerate if necessary
    time.sleep(2.0)

    print("\nIssuing FORMAT_STORAGE (this will erase all stored files)...")
    resp = send_command(ser, "FORMAT_STORAGE")
    print(resp.strip())

    print("\nVerifying file list is empty...")
    resp_list = send_command(ser, "LIST_FILES")
    print(resp_list.strip())

    print("\nIf no FILES entries remain besides headers, wipe succeeded. Power-cycle or reset the board to allow it to regenerate defaults if needed.")
    ser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
