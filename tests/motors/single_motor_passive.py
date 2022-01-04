from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port, Stop
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.B)

# Allocate the data logs.
DURATION = 10 * 1000
motor.log.start(DURATION)
motor.control.log.start(DURATION)

# Run the motor.
motor.run_time(500, 1000)
motor.dc(50)
wait(1000)
motor.stop()
wait(500)
motor.run_time(500, 1000)

# Wait so we can also log the stopped behavior.
wait(1000)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
motor.control.log.save("control.txt")
print("Done")
