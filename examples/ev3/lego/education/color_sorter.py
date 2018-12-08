#!/usr/bin/env pybricks-micropython

# import all the Pybricks stuff
from boot import *

# this stuff needs to get built-in so we don't have to do this manually
from os import path
from uev3dev.display import Display, ImageFile
from uev3dev.sound import Sound, SoundFile

brick.display = Display()
brick.sound = Sound()


class Image():
    _IMAGE_PATH = '/usr/share/images/ev3dev/mono'

    backward = ImageFile(path.join(_IMAGE_PATH, 'information/backward.png'),
                         brick.display)
    right = ImageFile(path.join(_IMAGE_PATH, 'information/right.png'),
                      brick.display)

    ev3 = ImageFile(path.join(_IMAGE_PATH, 'lego/ev3.png'), brick.display)


class Sound():
    _SOUND_PATH = '/usr/share/sounds/ev3dev'

    blue = SoundFile(path.join(_SOUND_PATH, 'colors/blue.wav'))
    green = SoundFile(path.join(_SOUND_PATH, 'colors/green.wav'))
    red = SoundFile(path.join(_SOUND_PATH, 'colors/red.wav'))
    yellow = SoundFile(path.join(_SOUND_PATH, 'colors/yellow.wav'))

    ready = SoundFile(path.join(_SOUND_PATH, 'system/ready.wav'))
# end of "built-in" stuff

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

    brick.display.reset_screen()

    # start with an empty list
    color_list = []

    # scan loop
    while len(color_list) < 8:
        # show arrow pointing to color sensor
        brick.display.image(Image.right, True, 0, 0)
        brick.display.text_grid(len(color_list), False, 0, 0, False, 2)

        # wait for center button to be pressed or color to be scanned
        while True:
            pressed = Button.center in brick.buttons()
            color = color_sensor.color()
            if pressed or color in POSSIBLE_COLORS:
                break

        if pressed:
            # if the button was pressed, end the loop early
            break
        else:
            # if a color was scanned, add it to the list
            brick.beep(1000, 100, 100)
            color_list.append(color)
            while color_sensor.color() not in (Color.black, None):
                pass
            brick.beep(2000, 100, 100)

            # point arrow to center button to ask if we are done
            brick.display.image(Image.backward, True, 0, 0)
            wait(2000)

    brick.sound.play_file(Sound.ready, 100, 0)

    brick.display.image(Image.ev3, True, 0, 0)

    # sorting loop
    for color in color_list:
        wait(1000)

        # make sure belt motor is in the starting position
        belt_motor.run(-500)
        while not touch_sensor.pressed():
            pass
        belt_motor.stop()

        # run belt motor to position based on the color
        if color == Color.blue:
            brick.sound.play_file(Sound.blue, 100, 0)
            belt_motor.run_angle(500, 10)
        elif color == Color.green:
            brick.sound.play_file(Sound.green, 100, 0)
            belt_motor.run_angle(500, 132)
        elif color == Color.yellow:
            brick.sound.play_file(Sound.yellow, 100, 0)
            belt_motor.run_angle(500, 360)
        elif color == Color.red:
            brick.sound.play_file(Sound.red, 100, 0)
            belt_motor.run_angle(500, 530)

        # eject the color block
        feed_motor.run_angle(1500, 90)
        feed_motor.run_angle(1500, -90)
