from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Direction
from pybricks.robotics import SpikeBase
from pybricks import version

print(version)

# Initialize base.
left_motor = Motor(Port.C)
right_motor = Motor(Port.D)
spike_base = SpikeBase(left_motor, right_motor)

# Allocate logs for motors and controller signals.
DURATION = 6000
left_motor.log.start(DURATION)
right_motor.log.start(DURATION)
spike_base.distance_control.log.start(DURATION)
spike_base.heading_control.log.start(DURATION)

# Turn in place, almost.
spike_base.tank_move_for_degrees(speed_left=250, speed_right=-247, angle=182)

# Wait so we can also log hold capability, then turn off the motor completely.
wait(100)
spike_base.stop()

# Transfer data logs.
print("Transferring data...")
left_motor.log.save("servo_left.txt")
right_motor.log.save("servo_right.txt")
spike_base.distance_control.log.save("control_distance.txt")
spike_base.heading_control.log.save("control_heading.txt")
print("Done")
