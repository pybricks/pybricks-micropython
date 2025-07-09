#!/usr/bin/env python

import argparse
import asyncssh
import asyncio
import csv
import datetime
import math
import matplotlib
import matplotlib.pyplot
import matplotlib.patches
import numpy
import os
import pathlib
import shutil
import subprocess
import sys

from pybricksdev.connections.pybricks import PybricksHub
from pybricksdev.connections.ev3dev import EV3Connection
from pybricksdev.connections.lego import REPLHub

from pybricksdev.ble import find_device


async def run_pybricks_script(script_name):
    """Runs a script on a hub with Pybricks firmware and awaits result."""

    # Connect to the hub.
    print("Searching for a hub.")
    hub = PybricksHub()
    address = await find_device()
    await hub.connect(address)
    print("Connected!")

    # Run the script and disconnect.
    await hub.run(str(script_name))
    await hub.disconnect()

    return hub.output


async def run_usb_repl_script(script_name):
    """Runs a script on a generic MicroPython REPL and awaits result."""

    # Connect to the hub.
    print("Searching for a hub via USB.")
    hub = REPLHub()
    await hub.connect()
    await hub.reset_hub()
    print("Entered REPL Mode.")

    # Run the script and disconnect.
    await hub.run(script_name)
    await hub.disconnect()
    return hub.output


async def run_ev3dev_script(script_name, address):
    """Runs a script on a hub with ev3dev and awaits result."""

    # Connect to the hub.
    print("Searching for a hub.")
    hub = EV3Connection()
    await hub.connect(address)
    print("Connected!")

    # Delete old log data.
    for file in ("/home/robot/servo.txt", "/home/robot/control.txt"):
        try:
            await hub.client.sftp.remove(file)
        except asyncssh.sftp.SFTPNoSuchFile:
            pass

    # Run script.
    await hub.run(str(script_name))

    # Retrieve log data.
    try:
        await hub.get("servo.txt", build_dir / "servo.txt")
        await hub.get("control.txt", build_dir / "control.txt")
    except asyncssh.sftp.SFTPNoSuchFile:
        pass
    await hub.disconnect()


def get_data(path):
    """Gets data columns from a comma separated file."""
    with open(path) as f:
        reader = csv.reader(f, delimiter=",")
        data = numpy.array([[int(x) for x in rec] for rec in reader])
    time = data[:, 0]
    return time, data


def gradient(data, time, smooth=8):
    """Computes a simple gradient from sampled data."""
    speed = []
    for i, t in enumerate(time):
        start = max(i - smooth, 0)
        end = min(i + smooth, len(time) - 1)
        speed.append((data[end] - data[start]) / (time[end] - time[start]))
    return numpy.array(speed)


def plot_status(axis, data, values, label):
    """Draws a status value in an existing axis as a horizontal color bar."""

    # Reshape to add finite height dimension
    data = data.reshape((1, len(data)))

    # Draw as an image.
    im = axis.imshow(
        data,
        vmin=min(values.keys()),
        vmax=10,
        cmap="tab10",
        aspect="auto",
        label="actuation",
        interpolation="nearest",
    )

    # Show status color legend
    colors = [im.cmap(im.norm(value)) for value in values.keys()]
    patches = [
        matplotlib.patches.Patch(color=colors[i], label="{l}".format(l=values[i]))
        for i in range(len(values.keys()))
    ]
    axis.legend(handles=patches, ncol=len(values), loc=1, bbox_to_anchor=(1, 0.8))

    # Drop tick values and add y label.
    axis.set_xticklabels(labels="")
    axis.set_yticklabels(labels="")
    axis.set_ylabel(label)


def plot_servo_data(time, data, build_dir, subtitle=None):
    """Plots data for a servo motor."""
    # Get loop time, convert ticks to ms.
    wall_time = data[:, 1] / 10
    wall_time_shifted = numpy.append(2 * wall_time[0] - wall_time[1], wall_time[0:-1])
    loop_time = wall_time - wall_time_shifted

    # Read state columns.
    count = data[:, 2]
    rate = data[:, 3]
    status_flags = data[:, 4]
    actuation_type = numpy.array([s & 0b0011 for s in status_flags])
    stalled = numpy.array([(1 if s & 0b0100 else 0) for s in status_flags])
    voltage = data[:, 5]
    count_est = data[:, 6]
    rate_est = data[:, 7]
    torque_feedback = data[:, 8]
    torque_feedforward = data[:, 9]
    observer_feedback_voltage = data[:, 10]

    title = "servo" if subtitle is None else "servo_" + subtitle

    figure, axes = matplotlib.pyplot.subplots(
        nrows=7, ncols=1, figsize=(15, 15), height_ratios=[1, 1, 1, 1, 0.05, 0.05, 0.5]
    )
    figure.suptitle(title, fontsize=20)

    position_ax, speed_ax, torque_ax, duty_ax, actuate_ax, stall_ax, time_ax = axes
    status_axes = (actuate_ax, stall_ax)

    position_ax.plot(time, count, drawstyle="steps-post", label="Reported count")
    position_ax.plot(time, count_est, drawstyle="steps-post", label="Observer")
    position_ax.set_ylabel("angle (deg)")

    speed_ax.plot(time, rate, drawstyle="steps-post", label="Reported rate")
    speed_ax.plot(time, rate_est, drawstyle="steps-post", label="Observer")
    speed_ax.plot(
        time,
        gradient(count, time / 1000),
        drawstyle="steps-post",
        label="Future count derivative",
    )
    speed_ax.set_ylabel("speed (deg/s)")

    torque_ax.plot(time, torque_feedback, label="Feedback", drawstyle="steps-post")
    torque_ax.plot(
        time, torque_feedforward, label="Feedforward", drawstyle="steps-post"
    )
    torque_ax.set_ylabel("Torque")

    duty_ax.plot(time, voltage, label="Motor", drawstyle="steps-post")
    duty_ax.plot(
        time,
        observer_feedback_voltage,
        label="Observer Feedback",
        drawstyle="steps-post",
    )
    duty_ax.plot(
        time,
        voltage + observer_feedback_voltage,
        label="Model voltage",
        drawstyle="steps-post",
    )
    duty_ax.set_ylabel("Voltage (mV)")
    duty_ax.set_ylim([-10000, 10000])

    time_ax.plot(time, loop_time, label="Loop time", drawstyle="steps-post")
    time_ax.set_ylabel("Time (ms)")
    time_ax.set_xlabel("time (ms)")
    loop_time_max = math.ceil((numpy.max(loop_time) + 1) / 5) * 5
    time_ax.set_ylim(0, loop_time_max)

    plot_status(
        actuate_ax, actuation_type, {0: "Coast", 1: "N/A", 2: "Voltage"}, "Act."
    )
    plot_status(stall_ax, stalled, {0: "No", 1: "Yes"}, "Stall.")

    for axis in axes:
        if axis not in status_axes:
            axis.grid(True)
            axis.set_xlim([time[0], time[-1]])
            axis.legend()

    figure.savefig(build_dir / (title + ".png"))


def plot_control_data(time, data, build_dir, subtitle=None):
    """Plots data for the controller."""
    maneuver_time = data[:, 1] / 10
    count = data[:, 2]
    rate = data[:, 3]
    status_flags = data[:, 4]
    actuation_type = numpy.array([s & 0b00011 for s in status_flags])
    stalled_or_paused = numpy.array(
        [(1 if s & 0b00100 else (2 if s & 0b10000 else 0)) for s in status_flags]
    )
    on_target = numpy.array([(1 if s & 0b01000 else 0) for s in status_flags])
    torque_total = data[:, 5]
    count_ref = data[:, 6]
    rate_ref = data[:, 7]
    count_est = data[:, 8]
    rate_est = data[:, 9]
    torque_p = data[:, 10]
    torque_i = data[:, 11]
    torque_d = data[:, 12]

    title = "control" if subtitle is None else "control_" + subtitle

    figure, axes = matplotlib.pyplot.subplots(
        nrows=8,
        ncols=1,
        figsize=(15, 15),
        height_ratios=[1, 1, 1, 1, 0.05, 0.05, 0.05, 0.5],
    )
    figure.suptitle(title, fontsize=20)

    (
        position_ax,
        error_ax,
        speed_ax,
        torque_ax,
        actuate_ax,
        stall_ax,
        done_ax,
        time_ax,
    ) = axes
    status_axes = (actuate_ax, stall_ax, done_ax)

    position_ax.plot(time, count, drawstyle="steps-post", label="Reported count")
    position_ax.plot(time, count_est, drawstyle="steps-post", label="Observer")
    position_ax.plot(time, count_ref, drawstyle="steps-post", label="Reference")
    position_ax.set_ylabel("angle (deg)")

    error_ax.plot(
        time, count_ref - count, drawstyle="steps-post", label="Reported error"
    )
    error_ax.plot(
        time, count_ref - count_est, drawstyle="steps-post", label="Estimated error"
    )
    error_ax.plot(
        time, count_est - count, drawstyle="steps-post", label="Estimation error"
    )
    error_ax.set_ylabel("angle error (deg)")

    speed_ax.plot(time, rate, drawstyle="steps-post", label="Reported rate")
    speed_ax.plot(time, rate_est, drawstyle="steps-post", label="Observer")
    speed_ax.plot(
        time,
        gradient(count, time / 1000),
        drawstyle="steps-post",
        label="Future count derivative",
    )
    speed_ax.plot(time, rate_ref, drawstyle="steps-post", label="Reference")
    speed_ax.set_ylabel("speed (deg/s)")

    torque_ax.plot(time, torque_p, label="P", drawstyle="steps-post")
    torque_ax.plot(time, torque_i, label="I", drawstyle="steps-post")
    torque_ax.plot(time, torque_d, label="D", drawstyle="steps-post")
    torque_ax.plot(time, torque_total, label="Total", drawstyle="steps-post")
    torque_ax.set_ylabel("torque")

    time_ax.plot(time, maneuver_time, label="t - t_0", drawstyle="steps-post")
    time_ax.set_ylabel("Maneuver time (ms)")
    time_ax.set_xlabel("time (ms)")

    ACTUATION_TYPES = {
        0: "Coast",
        1: "Brake",
        2: "Voltage",
        3: "Torque",
    }
    plot_status(actuate_ax, actuation_type, ACTUATION_TYPES, "Act")
    plot_status(stall_ax, stalled_or_paused, {0: "No", 1: "Yes", 2: "Paused"}, "Stall")
    plot_status(done_ax, on_target, {0: "No", 1: "Yes"}, "Done")

    for axis in axes:
        if axis not in status_axes:
            axis.grid(True)
            axis.set_xlim([time[0], time[-1]])
            axis.legend()

    figure.savefig(build_dir / (title + ".png"))


def make_plots(build_dir, show=True):
    # Plot single motor data if available.
    try:
        servo_time, servo_data = get_data(build_dir / "servo.txt")
        plot_servo_data(servo_time, servo_data, build_dir)
    except FileNotFoundError:
        pass

    # Plot control data if available.
    try:
        control_time, control_data = get_data(build_dir / "control.txt")
        plot_control_data(control_time, control_data, build_dir)
    except (IndexError, FileNotFoundError):
        pass

    # Plot drive base data if available.
    try:
        # Drive base case
        servo_time, servo_data = get_data(build_dir / "servo_left.txt")
        plot_servo_data(servo_time, servo_data, build_dir, "left")

        servo_time, servo_data = get_data(build_dir / "servo_right.txt")
        plot_servo_data(servo_time, servo_data, build_dir, "right")

        control_time, control_data = get_data(build_dir / "control_distance.txt")
        plot_control_data(control_time, control_data, build_dir, "distance")

        control_time, control_data = get_data(build_dir / "control_heading.txt")
        plot_control_data(control_time, control_data, build_dir, "heading")
    except FileNotFoundError:
        pass

    # If requested, show blocking windows with plots.
    if show:
        matplotlib.pyplot.show(block=True)


# Parse user argument.
parser = argparse.ArgumentParser(description="Run motor script and show log.")
parser.add_argument("file", help="Script to run")
parser.add_argument("--show", dest="show", default=False, action="store_true")
parser.add_argument("--address", dest="address")
parser.add_argument(
    "--target",
    dest="target",
    help="target type: %(choices)s",
    choices=["ble", "usb", "virtual", "ev3dev"],
    default="ble",
)
args = parser.parse_args()

# Sometimes we don't want to do new experiments but just visualize old data.
if pathlib.Path(args.file).is_dir():
    make_plots(pathlib.Path(args.file), args.show)
    sys.exit()

# Local paths and data directories.
time_string = datetime.datetime.now().strftime("-%Y-%m-%d-%H%M-%S")
script_base_name, _ = os.path.splitext(os.path.split(args.file)[-1])
test_dir = pathlib.Path(__file__).parent
build_dir = test_dir / "build" / (script_base_name + time_string)
pathlib.Path(build_dir).mkdir(parents=True, exist_ok=True)

# Copy script to data directory to archive experiment.
script_archive = build_dir / (script_base_name + ".py")
shutil.copyfile(args.file, script_archive)

# Configure matplotlib.
matplotlib.use("TkAgg")
matplotlib.interactive(True)

# Run the script on physical or virtual platform.
if args.target == "usb":
    hub_output = asyncio.run(run_usb_repl_script(script_archive))
elif args.target == "ble":
    hub_output = asyncio.run(run_pybricks_script(script_archive))
elif args.target == "ev3dev":
    hub_output = asyncio.run(run_ev3dev_script(script_archive, args.address))
else:
    top_path = (test_dir / "../..").absolute()
    bin_path = top_path / "bricks/virtualhub/build/virtualhub-micropython"
    if "PYTHONPATH" not in os.environ:
        os.environ["PYTHONPATH"] = str(top_path / "lib/pbio/cpython")
    if "PBIO_VIRTUAL_PLATFORM_MODULE" not in os.environ:
        os.environ["PBIO_VIRTUAL_PLATFORM_MODULE"] = "pbio_virtual.platform.turtle"
    result = subprocess.run(
        [bin_path, script_archive.absolute()],
        capture_output=True,
        cwd=build_dir.absolute(),
    )
    hub_output = (result.stdout or result.stderr).split(b"\n")
    for line in hub_output:
        print(line.decode())

# Save its standard output.
if hub_output is not None:
    with open(build_dir / "hub_output.txt", "wb") as f:
        for line in hub_output:
            f.write(line + b"\n")

# Visualize the data.
make_plots(build_dir, args.show)
