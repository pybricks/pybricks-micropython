#!/usr/bin/env python3
import argparse
import serial
import time

from mpybytes import get_bytes_from_file, get_bytes_from_str


def download_and_run(mpy_bytes):
    # Open serial port
    ser = serial.Serial('/dev/ttyACM0', baudrate=115200, interCharTimeout=1)

    # Get the mpy file size, divide into 4 bytes and send to hub
    size = len(mpy_bytes)
    sizebytes = [(size >> (3-i)*8) % 256 for i in range(4)]
    for b in sizebytes:
        ser.write(bytes([b]))
        time.sleep(0.05)

    # Wait/check ACK
    ser.read_until(b'Ready to receive.')
    time.sleep(0.1)

    # Split binary up in digestable chunks
    n = 1
    chunks = [mpy_bytes[i:i+n] for i in range(0, len(mpy_bytes), n)]
    print('Sending mpy of {0} bytes in {1} chunks of {2} bytes each'.format(size, len(chunks), n))

    # Send the data
    for chunk in chunks:
        ser.write(bytes(chunk))
        time.sleep(n/500)

    # TODO: Parse output
    time.sleep(0.5)
    print(str(ser.read(ser.inWaiting())))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run Pybricks scripts or commands over serial port.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--file', dest='file', nargs='?', const=1, type=str)
    group.add_argument('--string', dest='string', nargs='?', const=1, type=str)
    args = parser.parse_args()

    if args.file:
        bytearr = get_bytes_from_file(args.file)

    if args.string:
        bytearr = get_bytes_from_str(args.string)

    download_and_run(bytearr)
