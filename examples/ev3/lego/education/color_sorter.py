#!/usr/bin/env pybricks-micropython

# import all the Pybricks stuff
from pybricks import ev3brick as brick
from pybricks.ev3devices import Motor, TouchSensor, ColorSensor, InfraredSensor, UltrasonicSensor, GyroSensor
from pybricks.parameters import Port, Stop, Direction, Completion, Button, Color, Image, Align, Sound
from pybricks.tools import wait, StopWatch

POSSIBLE_COLORS = (Color.red, Color.green, Color.blue, Color.yellow)

belt_motor = Motor(Port.D)
feed_motor = Motor(Port.A)
touch_sensor = TouchSensor(Port.S1)
color_sensor = ColorSensor(Port.S3)

# Main loop
while True:
    # get the color block feed motor in the correct starting position
    feed_motor.run_stalled(120)
    feed_motor.run_angle(450, -180)

    # get the conveyor belt motor in the correct starting position
    belt_motor.run(-500)
    while not touch_sensor.pressed():
        pass
    belt_motor.stop()
    wait(1000)
    belt_motor.reset_angle()

    brick.display.clear()

    # start with an empty list
    color_list = []

    # scan loop
    while len(color_list) < 8:
        # show arrow pointing to color sensor
        brick.display.image(Image.right)
        brick.display.text(len(color_list))

        # wait for center button to be pressed or color to be scanned
        while True:
            pressed = Button.center in brick.buttons()
            color = color_sensor.color()
            if pressed or color in (Color.red, Color.green, Color.blue, Color.yellow):
                break

        if pressed:
            # if the button was pressed, end the loop early
            break
        else:
            # if a color was scanned, add it to the list
            brick.sound.beep(1000, 100, 100)
            color_list.append(color)
            while color_sensor.color() in POSSIBLE_COLORS:
                pass
            brick.sound.beep(2000, 100, 100)

            # point arrow to center button to ask if we are done
            brick.display.image(Image.backward)
            wait(2000)

    brick.sound.file(Sound.ready)

    brick.display.image(Image.ev3)

    # sorting loop
    for color in color_list:
        wait(1000)

        # run belt motor to position based on the color
        if color == Color.blue:
            brick.sound.file(Sound.blue)
            belt_motor.run_target(500, 10)
        elif color == Color.green:
            brick.sound.file(Sound.green)
            belt_motor.run_target(500, 132)
        elif color == Color.yellow:
            brick.sound.file(Sound.yellow)
            belt_motor.run_target(500, 360)
        elif color == Color.red:
            brick.sound.file(Sound.red)
            belt_motor.run_target(500, 530)

        # eject the color block
        feed_motor.run_angle(1500, 90)
        feed_motor.run_angle(1500, -90)
