from pybricks.pupdevices import Motor
from pybricks.tools import wait
from pybricks.parameters import Port
from pybricks import version

print(version)

motor = Motor(Port.A)

DURATION = 2000
UNDERSAMPLE = 1

motor.log.start(DURATION, UNDERSAMPLE)
motor.control.log.start(DURATION, UNDERSAMPLE)


motor.run_target(500, 360)

wait(500)
motor.stop()

print("Transferring data...")
motor.log.save("log_single_motor_servo.txt")
motor.control.log.save("log_single_motor_control.txt")
print("Done")
