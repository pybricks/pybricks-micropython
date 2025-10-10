#!/usr/bin/env python

# This program receives angle data from the simulated pbio motor driver. The
# driver outputs it at intervals of 40 ms (25 fps).

from collections import namedtuple
import socket

HOST = "127.0.0.1"
PORT = 5002

angles = []

print(f"Listening on {HOST}:{PORT}")


# Wait for a connection once and writes results when simulator disconnects.
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()
    print(f"Client connected: {addr}")
    with conn:
        # receive until client disconnects
        buffer = b""
        while True:
            data = conn.recv(1024)
            if not data:
                print("Client disconnected.")
                break
            buffer += data
            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                values = list(map(int, line.decode().strip().split()))
                print("Angles:", values)
                angles.append(values)


if len(angles) < 1:
    exit()

duration = (len(angles) - 1) * 0.04
increment = 100 / len(angles)

# Frame info for each rotatary components
InfoTuple = namedtuple("FrameInfo", ("name", "index", "gearing", "width", "x", "y"))
FRAME_INFO = (
    InfoTuple("shaft", index=4, gearing=1, width=152, x=-453, y=212),
    InfoTuple("stall", index=2, gearing=1, width=344, x=-455, y=-311),
    InfoTuple("gear-drive", index=5, gearing=-1, width=54, x=243, y=194),
    InfoTuple("gear-follow", index=5, gearing=3, width=149, x=337, y=194),
    InfoTuple("wheel-left", index=0, gearing=-1, width=222, x=542, y=-344),
    InfoTuple("wheel-right", index=1, gearing=1, width=222, x=367, y=-169),
)

# Write the CSS component with frames.
with open("lib/pbio/test/results/frames.css", "w") as frame_file:
    for info in FRAME_INFO:
        # CSS rows for each frame.
        frames = "".join(
            [
                f"{i * 100 // (len(angles) - 1)}% {{transform: translate({info.x}px, {info.y}px) rotate( {int(row[info.index]) // info.gearing}deg );}}\n"
                for i, row in enumerate(angles)
            ]
        )

        # Main css for this class.
        css = f"""
        .{info.name} {{
            width: {info.width}px;
            display: inline-block;
            animation: {info.name}-frames {duration}s 1s linear;
            animation-fill-mode: forwards;
            transform: translate({info.x}px, {info.y}px) rotate( {int(angles[0][info.index]) // info.gearing}deg );
            transform-origin: 50% 50%;
            position:absolute;
        }}

        @keyframes {info.name}-frames {{
        {frames}
        }}
        """
        frame_file.write(css)
