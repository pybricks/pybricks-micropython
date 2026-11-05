#!/usr/bin/env python
"""
Runs a motor test script on the hub that is currently connected with
``pb connect``, saves the logs it prints, and plots them.

Pass a results directory instead of a script to plot old data again.
"""

import argparse
import datetime
import math
import pathlib
import shutil
import subprocess
import sys

import matplotlib.patches
import matplotlib.pyplot
import numpy

# Markers printed by Logger.save() around the contents of each log file.
LOG_FILE_START = "PB_OF:"
LOG_FILE_END = "PB_EOF"


def run_script(script, build_dir, hub=None):
    """Runs a script with pb run, saving its output and any log files."""

    hub_args = [] if hub is None else ["--hub", hub]
    command = ["pb", "run", "--wait", str(script)] + hub_args

    log_file = None

    with (
        open(build_dir / "hub_output.txt", "w") as output,
        subprocess.Popen(command, stdout=subprocess.PIPE, text=True) as process,
    ):
        try:
            for line in process.stdout:
                line = line.rstrip("\r\n")
                print(line)
                output.write(line + "\n")

                if line.startswith(LOG_FILE_START):
                    if log_file is not None:
                        print(f"Warning: {log_file.name} was incomplete.")
                        log_file.close()
                    log_file = open(build_dir / line[len(LOG_FILE_START) :], "w")
                elif line == LOG_FILE_END and log_file is not None:
                    log_file.close()
                    log_file = None
                elif log_file is not None:
                    log_file.write(line + "\n")
        except KeyboardInterrupt:
            print("Interrupted. Stopping the program on the hub.")
            process.wait()
            subprocess.run(["pb", "stop"] + hub_args, check=False)
        finally:
            if log_file is not None:
                print(f"Warning: {log_file.name} was incomplete.")
                log_file.close()

    if process.returncode:
        print(f"Warning: pb run exited with code {process.returncode}.")


def get_data(path):
    """Gets data columns from a comma separated file."""
    return numpy.loadtxt(path, delimiter=",", dtype=int, ndmin=2)


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


def plot_servo_data(data, title):
    """Plots data for a servo motor. Columns as in pbio/src/servo.c."""
    (
        time,
        wall_time,
        count,
        rate,
        status_flags,
        voltage,
        count_est,
        rate_est,
        torque_feedback,
        torque_feedforward,
        observer_feedback_voltage,
    ) = data.T

    # Get loop time, convert ticks to ms.
    wall_time = wall_time / 10
    wall_time_shifted = numpy.append(2 * wall_time[0] - wall_time[1], wall_time[0:-1])
    loop_time = wall_time - wall_time_shifted

    actuation_type = status_flags & 0b0011
    stalled = (status_flags & 0b0100) >> 2

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

    return figure


def plot_control_data(data, title):
    """Plots data for the controller. Columns as in pbio/src/control.c."""
    (
        time,
        maneuver_time,
        count,
        rate,
        status_flags,
        torque_total,
        count_ref,
        rate_ref,
        count_est,
        rate_est,
        torque_p,
        torque_i,
        torque_d,
    ) = data.T

    maneuver_time = maneuver_time / 10
    actuation_type = status_flags & 0b00011
    stalled_or_paused = numpy.where(
        status_flags & 0b00100, 1, numpy.where(status_flags & 0b10000, 2, 0)
    )
    on_target = (status_flags & 0b01000) >> 3

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

    return figure


# Plot functions by log file name prefix, e.g. servo.txt or control_heading.txt.
PLOTTERS = {
    "servo": plot_servo_data,
    "control": plot_control_data,
}


def make_plots(build_dir, show=False):
    """Plots all known log files in the given directory."""
    for path in sorted(build_dir.glob("*.txt")):
        plotter = PLOTTERS.get(path.stem.split("_")[0])
        if plotter is None:
            continue
        try:
            figure = plotter(get_data(path), path.stem)
        except (ValueError, IndexError) as e:
            print(f"Warning: could not plot {path.name}: {e}")
            continue
        figure.savefig(build_dir / (path.stem + ".png"))

    # If requested, show blocking windows with plots.
    if show:
        matplotlib.pyplot.show()


parser = argparse.ArgumentParser(description="Run motor script and show log.")
parser.add_argument("file", help="Script to run, or results directory to plot")
parser.add_argument("--show", action="store_true", help="show plots in a window")
parser.add_argument("--hub", help="hub name, defaults to the one in use by pb")
args = parser.parse_args()

# Sometimes we don't want to do new experiments but just visualize old data.
if pathlib.Path(args.file).is_dir():
    make_plots(pathlib.Path(args.file), args.show)
    sys.exit()

# Local paths and data directories.
script = pathlib.Path(args.file)
time_string = datetime.datetime.now().strftime("-%Y-%m-%d-%H%M-%S")
build_dir = pathlib.Path(__file__).parent / "build" / (script.stem + time_string)
build_dir.mkdir(parents=True, exist_ok=True)

# Copy script to data directory to archive experiment.
shutil.copyfile(script, build_dir / script.name)

run_script(script, build_dir, args.hub)
make_plots(build_dir, args.show)
