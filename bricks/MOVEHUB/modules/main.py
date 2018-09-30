from hub import PORT_B, PORT_A
from hub import MovehubMotor

# from MovehubMotor import STOP_COAST, STOP_HOLD, STOP_BRAKE, DIR_NORMAL, DIR_INVERTED // This import doesn't work yet
from time import sleep

motor = MovehubMotor(PORT_B)

motor.run_angle(500, 270, 2, False)

sleep(3)

motor.run_target(1000, 0, 0, False)

sleep(3)
