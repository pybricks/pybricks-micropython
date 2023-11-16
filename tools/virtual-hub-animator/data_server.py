#!/usr/bin/env python

# This program receives angle data from the simulated pbio motor driver and
# serves it on a socket.

import eventlet
import socketio
import threading

sio = socketio.Server(cors_allowed_origins="*", async_mode="eventlet")
app = socketio.WSGIApp(
    sio, static_files={"/": {"content_type": "text/html", "filename": "index.html"}}
)


@sio.event
def connect(sid, environ):
    print("Frontend connected!", sid)


@sio.event
def disconnect(sid):
    print("Frontend disconnected!", sid)


@sio.on("hubEventData")
def handle_message(id, data):
    """event listener when client types a message"""
    print("Data from the front end:", data)


motor_angles = [0, 0, 0, 0, 0, 0]


def socket_send_data_task():
    while True:
        sio.sleep(0.04)
        sio.emit("hubStateData", {"data": motor_angles})


def server_task():
    sio.start_background_task(socket_send_data_task)
    eventlet.wsgi.server(eventlet.listen(("", 5001)), app)


threading.Thread(target=server_task, daemon=True).start()

# Get live output from process.
while True:
    try:
        motor_angles = input().strip(" \r").split(" ")
    except EOFError:
        break
