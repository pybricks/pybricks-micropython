from hub import PORT_B
from hub import MovehubMotor
# from MovehubMotor import STOP_COAST, STOP_HOLD, STOP_BRAKE, DIR_NORMAL, DIR_INVERTED // This import doesn't work yet
from time import sleep

motor = MovehubMotor(PORT_B)
motor.duty(50)
sleep(0.2)
print(motor.angle())

motor.run_time(-500, 2, 0, False)

# Wait a while before restarting the script
sleep(5)
