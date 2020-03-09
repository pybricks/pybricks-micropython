#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

import argparse
import serial
import time
from mpybytes import mpy_bytes_from_file, mpy_bytes_from_str


def send_message(ser, data):
    """Send bytes to the hub, and check if reply matches checksum."""

    # Initial checksum
    checksum = 0

    # Send data with brief pause between each byte
    for b in data:
        checksum ^= b
        ser.write(bytes([b]))
        time.sleep(0.002)

    # Give hub time to send its checksum, then read it
    time.sleep(0.1)
    reply = ser.read()

    # Raise errors if we did not get the checksum we wanted
    if not reply:
        raise OSError("Did not receive reply.")

    if checksum != reply[-1]:
        raise ValueError("Did not receive expected checksum.")


def download_and_run(device, mpy_bytes):
    """Split bytes from an MPY file into chunks and send to the hub."""

    # Open serial port
    ser = serial.Serial(device, baudrate=115200, timeout=0)

    # Get the mpy file size as 4 bytes
    send_message(ser, len(mpy_bytes).to_bytes(4, byteorder="big"))

    # Split binary up in digestable chunks
    n = 100
    chunks = [mpy_bytes[i : i + n] for i in range(0, len(mpy_bytes), n)]

    # Send the data
    for chunk in chunks:
        send_message(ser, chunk)

    # Give hub time to start program
    time.sleep(0.2)

    # Read first part of response
    data = ser.read_all()
    printed = 0

    # Read status to see if program started
    RUNNING = b">>>> RUNNING"
    IDLE = b">>>> IDLE"
    if RUNNING not in data:
        raise OSError("Failed to run program")

    # Read from serial until idle status
    while True:
        # Append new data
        data += ser.read_all()

        # Split into lines, printing anything new
        text = data.decode().split("\r\n")
        while printed < len(text):
            print(text[printed - 1])
            printed += 1

        # Sleep and exit when done
        time.sleep(0.1)
        if IDLE in data:
            break

    # Save log if detected in output
    start_key = b"PB_OF"
    end_key = b"PB_EOF"
    if start_key in data and end_key in data:

        # Get data between keys
        start = data.index(start_key) + len(start_key) + 1
        end = data.index(end_key) - 2
        log_data = data[start:end]

        # Extract file name and data
        lines = log_data.decode().split("\r\n")
        with open(lines[0], "w") as f:
            print(*lines[1:], sep="\n", file=f)


if __name__ == "__main__":
    examples = """Examples:

    python3 tools/runserial.py --dev /dev/ttyACM0 --string 'print("Hello!")'
    python3 tools/runserial.py --dev /dev/ttyACM0 --file ~/helloworld.py
    """

    parser = argparse.ArgumentParser(
        description="Run Pybricks scripts or commands over serial port.",
        epilog=examples,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument("--mpy_cross", dest="mpy_cross", nargs="?", type=str, required=True)
    parser.add_argument("--dev", dest="device", nargs="?", type=str, required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", dest="file", nargs="?", const=1, type=str)
    group.add_argument("--string", dest="string", nargs="?", const=1, type=str)
    args = parser.parse_args()

    if args.file:
        bytearr = mpy_bytes_from_file(args.mpy_cross, args.file)

    if args.string:
        bytearr = mpy_bytes_from_str(args.mpy_cross, args.string)

    download_and_run(args.device, bytearr)
