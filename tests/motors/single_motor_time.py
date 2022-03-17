from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Start the motor at 1000 deg/s.
motor.run(1000)
wait(1000)

# Allocate the data logs.
DURATION = 4000
motor.log.start(DURATION)
motor.control.log.start(DURATION)
wait(500)

# Run the motor with a short timed command.
motor.run_time(500, 1200, wait=False)
print(motor.control.trajectory())

# Wait so we can also log the stopped behavior.
wait(2000)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
