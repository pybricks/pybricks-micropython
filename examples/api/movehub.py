# Configure hub motor A as default
left = Motor(Port.A)

# Configure motor B as inverted and with two fictitious gear trains.
# The total reduction here is (36/12*12/36*24/12)*(16/8)=4, but the user
# won't actually have to do any math. They can just count (or look up)
# the number of teeth on the gears and enter them as arguments.
right = Motor(Port.B, Dir.inverted, [12, 36, 12, 24], [8, 16])

# A simple 180 maneuver to a fixed position
left.run_target(500, 180)

# An additional 90 degree maneuver
left.run_angle(500, 90)

# Rotate mechanism output axle attached to motor B by 90 degrees.
# This automatically compensates for gear train: the motor really turns 360 degrees at 2000 deg/s
right.run_angle(500, 90)

# Back to the start and hold there (See #17 TODO that fixes high pitch due to too low duty)
left.run_target(500, 0, Stop.hold)

# Reinit motor B with default settings, just like A
right = Motor(Port.B)

# run both motors. This will become simpler and more precise (synchronized) once we implement the DriveBase class driver
left.run_time(500, 3000, Stop.brake, Run.background)
right.run_time(500, 3000, Stop.brake)

# Run then forced stop
left.run(500)
wait(1000)
left.stop()
