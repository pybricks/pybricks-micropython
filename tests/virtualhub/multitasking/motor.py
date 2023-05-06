from pybricks.pupdevices import Motor
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase
from pybricks.tools import wait, Task, run_task

# Initialize devices as usual.
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)
stall_motor = Motor(Port.C)


# Task to drive in a square.
async def square():
    for side in range(4):
        # Drive forward and then turn.
        await drive_base.straight(200)
        await drive_base.turn(90)


# Task to center a motor.
async def center():
    left = await stall_motor.run_until_stalled(-200)
    right = await stall_motor.run_until_stalled(200)
    stall_motor.reset_angle((right - left) / 2)
    await stall_motor.run_target(500, 0)
    print("Motor centered!")


# Hello world task.
async def hello(name):
    print("Hello!")
    await wait(2000)
    print(name)


# This is the main program
async def main():
    print("Running multiple tasks at once!")
    await Task(square(), center(), hello("Pybricks"))
    print("You can also just run one task.")
    await hello("World!")


# Run the main program.
run_task(main())
