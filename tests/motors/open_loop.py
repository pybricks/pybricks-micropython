try:
    from pybricks.pupdevices import Motor
except ImportError:
    from pybricks.ev3devices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

# Initialize the motor.
motor = Motor(Port.A)

# Allocate the data logs.
motor.log.start(6000)

for i in range(11):
    print(motor.angle())
    motor.dc(i * 10)
    wait(500)

# Wait so we can also log the stopped behavior.
motor.dc(0)
wait(500)
motor.stop()

# Transfer data logs.
print("Transferring data...")
motor.log.save("servo.txt")
print("Done")
