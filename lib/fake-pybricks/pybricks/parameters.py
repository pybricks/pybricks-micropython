"""Constant parameters/arguments for the Pybricks API."""

from enum import Enum


class Color(Enum):
    """Light or surface color.

    .. data:: black
    .. data:: blue
    .. data:: green
    .. data:: yellow
    .. data:: red
    .. data:: white
    .. data:: brown
    .. data:: orange
    .. data:: purple
    """

    black = 1
    blue = 2
    green = 3
    yellow = 4
    red = 5
    white = 6
    brown = 7
    orange = 8
    purple = 9


class Port(Enum):
    """Port on the EV3 Programmable Brick.

    Motor ports:

    .. data:: A
    .. data:: B
    .. data:: C
    .. data:: D

    Sensor ports:

    .. data:: S1
    .. data:: S2
    .. data:: S3
    .. data:: S4
    """

    # Generic motor/sensor ports
    A = ord('A')
    B = ord('B')
    C = ord('C')
    D = ord('D')

    # NXT/EV3 sensor ports
    S1 = ord('1')
    S2 = ord('2')
    S3 = ord('3')
    S4 = ord('4')


class Stop(Enum):
    """Action after the motor stops: coast, brake, or hold.

    .. data:: COAST

        Let the motor move freely.

    .. data:: BRAKE

        Passively resist small external forces.

    .. data:: HOLD

        Keep controlling the motor to hold it at the commanded angle. This is only available on motors with encoders.

    The stop type defines the resistance to motion after coming to standstill:

    +-----------+-------------+------------------------------------------+
    |Parameter  | Resistance  | Physical meaning                         |
    +===========+=============+==========================================+
    |Stop.COAST | low         | Friction                                 |
    +-----------+-------------+------------------------------------------+
    |Stop.BRAKE | medium      | Friction + Torque opposite to motion     |
    +-----------+-------------+------------------------------------------+
    |Stop.HOLD  | high        | Friction + Torque to hold commanded angle|
    +-----------+-------------+------------------------------------------+

    """

    COAST = 0
    BRAKE = 1
    HOLD = 2


class Direction():
    """Rotational direction for positive speed values: clockwise or counterclockwise.

    .. data:: CW

        A positive speed value should make the motor move clockwise.

    .. data:: CCW

        A positive speed value should make the motor move counterclockwise.

    For all motors, this is defined when looking at the shaft, just like looking at a clock.

    For NXT or EV3 motors, make sure to look at the motor with the red/orange shaft to the lower right.

    +-------------------+-------------------+------------------+
    | Parameter         | Positive speed    | Negative speed   |
    +===================+===================+==================+
    | Direction.CW      | clockwise         | counterclockwise |
    +-------------------+-------------------+------------------+
    | Direction.CCW     | counterclockwise  | clockwise        |
    +-------------------+-------------------+------------------+

    ::


         Medium EV3 Motor:


         counterclockwise          clockwise
               ____                 _____
              /                          \\
             /       _____________        \\
            /       /              \       \\
            |      |        _       |       |
            |      |     __| |__    |       |
            v      |    |__ o __|   |       v
                   |       |_|      |
                   |                |
                    \______________/


         Large EV3 motor:

              ________
             /         \        ___    ___
            _|          \      /          \\
            |             ----/------      \\
            counterclockwise  |    __\__    |  clockwise
              \__________     v  /      \   v
                          -------|  +   |
                                 \_____/


    """

    CW = 0
    CCW = 1


class Button(Enum):
    """Buttons on a brick or remote:

    .. data:: LEFT_DOWN
    .. data:: DOWN
    .. data:: RIGHT_DOWN
    .. data:: LEFT
    .. data:: CENTER
    .. data:: RIGHT
    .. data:: LEFT_UP
    .. data:: UP
    .. data:: BEACON
    .. data:: RIGHT_UP

    +-----------+----------+-----------+
    |           |          |           |
    | LEFT_UP   |UP/BEACON | RIGHT_UP  |
    |           |          |           |
    +-----------+----------+-----------+
    |           |          |           |
    | LEFT      |  CENTER  | RIGHT     |
    |           |          |           |
    +-----------+----------+-----------+
    |           |          |           |
    | LEFT_DOWN |   DOWN   | RIGHT_DOWN|
    |           |          |           |
    +-----------+----------+-----------+
    """

    LEFT_DOWN = 1
    DOWN = 2
    RIGHT_DOWN = 3
    LEFT = 4
    CENTER = 5
    RIGHT = 6
    LEFT_UP = 7
    UP = 8
    BEACON = 8
    RIGHT_UP = 9


class Align():
    """Alignment of an image on the display.

    .. data:: BOTTOM_LEFT
    .. data:: BOTTOM
    .. data:: BOTTOM_RIGHT
    .. data:: LEFT
    .. data:: CENTER
    .. data:: RIGHT
    .. data:: TOP_LEFT
    .. data:: TOP
    .. data:: TOP_RIGHT
    """
    BOTTOM_LEFT = 1
    BOTTOM = 2
    BOTTOM_RIGHT = 3
    LEFT = 4
    CENTER = 5
    RIGHT = 6
    TOP_LEFT = 7
    TOP = 8
    TOP_RIGHT = 9


class Sound():
    """Paths to standard EV3 sounds.

    Expressions

    .. data:: shouting
    .. data:: cheering
    .. data:: crying
    .. data:: ouch
    .. data:: laughing_2
    .. data:: sneezing
    .. data:: smack
    .. data:: boing
    .. data:: boo
    .. data:: uh_oh
    .. data:: snoring
    .. data:: kung_fu
    .. data:: fanfare
    .. data:: crunching
    .. data:: magic_wand
    .. data:: laughing_1

    Information

    .. data:: left
    .. data:: backwards
    .. data:: right
    .. data:: object
    .. data:: color
    .. data:: flashing
    .. data:: error
    .. data:: error_alarm
    .. data:: down
    .. data:: forward
    .. data:: activate
    .. data:: searching
    .. data:: touch
    .. data:: up
    .. data:: analyze
    .. data:: stop
    .. data:: detected
    .. data:: turn
    .. data:: start

    Communication

    .. data:: morning
    .. data:: ev3
    .. data:: go
    .. data:: good_job
    .. data:: okey_dokey
    .. data:: good
    .. data:: no
    .. data:: thank_you
    .. data:: yes
    .. data:: game_over
    .. data:: okay
    .. data:: sorry
    .. data:: bravo
    .. data:: goodbye
    .. data:: hi
    .. data:: hello
    .. data:: mindstorms
    .. data:: lego
    .. data:: fantastic

    Movements

    .. data:: speed_idle
    .. data:: speed_down
    .. data:: speed_up

    Color

    .. data:: brown
    .. data:: green
    .. data:: black
    .. data:: white
    .. data:: red
    .. data:: blue
    .. data:: yellow

    Mechanical

    .. data:: tick_tack
    .. data:: horn_1
    .. data:: backing_alert
    .. data:: motor_idle
    .. data:: air_release
    .. data:: airbrake
    .. data:: ratchet
    .. data:: motor_stop
    .. data:: horn_2
    .. data:: laser
    .. data:: sonar
    .. data:: motor_start

    Animals

    .. data:: insect_buzz_2
    .. data:: elephant_call
    .. data:: snake_hiss
    .. data:: dog_bark_2
    .. data:: dog_whine
    .. data:: insect_buzz_1
    .. data:: dog_sniff
    .. data:: t_rex_roar
    .. data:: insect_chirp
    .. data:: dog_growl
    .. data:: snake_rattle
    .. data:: dog_bark_1
    .. data:: cat_purr

    Numbers

    .. data:: zero
    .. data:: one
    .. data:: two
    .. data:: three
    .. data:: four
    .. data:: five
    .. data:: six
    .. data:: seven
    .. data:: eight
    .. data:: nine
    .. data:: ten

    System

    .. data:: ready
    .. data:: confirm
    .. data:: general_alert
    .. data:: click
    .. data:: overpower

    """

    _basepath = '/usr/share/sounds/ev3dev/'
    shouting = _basepath + 'expressions/shouting.wav'
    cheering = _basepath + 'expressions/cheering.wav'
    crying = _basepath + 'expressions/crying.wav'
    ouch = _basepath + 'expressions/ouch.wav'
    laughing_2 = _basepath + 'expressions/laughing_2.wav'
    sneezing = _basepath + 'expressions/sneezing.wav'
    smack = _basepath + 'expressions/smack.wav'
    boing = _basepath + 'expressions/boing.wav'
    boo = _basepath + 'expressions/boo.wav'
    uh_oh = _basepath + 'expressions/uh-oh.wav'
    snoring = _basepath + 'expressions/snoring.wav'
    kung_fu = _basepath + 'expressions/kung_fu.wav'
    fanfare = _basepath + 'expressions/fanfare.wav'
    crunching = _basepath + 'expressions/crunching.wav'
    magic_wand = _basepath + 'expressions/magic_wand.wav'
    laughing_1 = _basepath + 'expressions/laughing_1.wav'
    left = _basepath + 'information/left.wav'
    backwards = _basepath + 'information/backwards.wav'
    right = _basepath + 'information/right.wav'
    object = _basepath + 'information/object.wav'
    color = _basepath + 'information/color.wav'
    flashing = _basepath + 'information/flashing.wav'
    error = _basepath + 'information/error.wav'
    error_alarm = _basepath + 'information/error_alarm.wav'
    down = _basepath + 'information/down.wav'
    forward = _basepath + 'information/forward.wav'
    activate = _basepath + 'information/activate.wav'
    searching = _basepath + 'information/searching.wav'
    touch = _basepath + 'information/touch.wav'
    up = _basepath + 'information/up.wav'
    analyze = _basepath + 'information/analyze.wav'
    stop = _basepath + 'information/stop.wav'
    detected = _basepath + 'information/detected.wav'
    turn = _basepath + 'information/turn.wav'
    start = _basepath + 'information/start.wav'
    morning = _basepath + 'communication/morning.wav'
    ev3 = _basepath + 'communication/ev3.wav'
    go = _basepath + 'communication/go.wav'
    good_job = _basepath + 'communication/good_job.wav'
    okey_dokey = _basepath + 'communication/okey-dokey.wav'
    good = _basepath + 'communication/good.wav'
    no = _basepath + 'communication/no.wav'
    thank_you = _basepath + 'communication/thank_you.wav'
    yes = _basepath + 'communication/yes.wav'
    game_over = _basepath + 'communication/game_over.wav'
    okay = _basepath + 'communication/okay.wav'
    sorry = _basepath + 'communication/sorry.wav'
    bravo = _basepath + 'communication/bravo.wav'
    goodbye = _basepath + 'communication/goodbye.wav'
    hi = _basepath + 'communication/hi.wav'
    hello = _basepath + 'communication/hello.wav'
    mindstorms = _basepath + 'communication/mindstorms.wav'
    lego = _basepath + 'communication/lego.wav'
    fantastic = _basepath + 'communication/fantastic.wav'
    speed_idle = _basepath + 'movements/speed_idle.wav'
    speed_down = _basepath + 'movements/speed_down.wav'
    speed_up = _basepath + 'movements/speed_up.wav'
    brown = _basepath + 'colors/brown.wav'
    green = _basepath + 'colors/green.wav'
    black = _basepath + 'colors/black.wav'
    white = _basepath + 'colors/white.wav'
    red = _basepath + 'colors/red.wav'
    blue = _basepath + 'colors/blue.wav'
    yellow = _basepath + 'colors/yellow.wav'
    tick_tack = _basepath + 'mechanical/tick_tack.wav'
    horn_1 = _basepath + 'mechanical/horn_1.wav'
    backing_alert = _basepath + 'mechanical/backing_alert.wav'
    motor_idle = _basepath + 'mechanical/motor_idle.wav'
    air_release = _basepath + 'mechanical/air_release.wav'
    airbrake = _basepath + 'mechanical/airbrake.wav'
    ratchet = _basepath + 'mechanical/ratchet.wav'
    motor_stop = _basepath + 'mechanical/motor_stop.wav'
    horn_2 = _basepath + 'mechanical/horn_2.wav'
    laser = _basepath + 'mechanical/laser.wav'
    sonar = _basepath + 'mechanical/sonar.wav'
    motor_start = _basepath + 'mechanical/motor_start.wav'
    insect_buzz_2 = _basepath + 'animals/insect_buzz_2.wav'
    elephant_call = _basepath + 'animals/elephant_call.wav'
    snake_hiss = _basepath + 'animals/snake_hiss.wav'
    dog_bark_2 = _basepath + 'animals/dog_bark_2.wav'
    dog_whine = _basepath + 'animals/dog_whine.wav'
    insect_buzz_1 = _basepath + 'animals/insect_buzz_1.wav'
    dog_sniff = _basepath + 'animals/dog_sniff.wav'
    t_rex_roar = _basepath + 'animals/t-rex_roar.wav'
    insect_chirp = _basepath + 'animals/insect_chirp.wav'
    dog_growl = _basepath + 'animals/dog_growl.wav'
    snake_rattle = _basepath + 'animals/snake_rattle.wav'
    dog_bark_1 = _basepath + 'animals/dog_bark_1.wav'
    cat_purr = _basepath + 'animals/cat_purr.wav'
    eight = _basepath + 'numbers/eight.wav'
    seven = _basepath + 'numbers/seven.wav'
    six = _basepath + 'numbers/six.wav'
    four = _basepath + 'numbers/four.wav'
    ten = _basepath + 'numbers/ten.wav'
    one = _basepath + 'numbers/one.wav'
    two = _basepath + 'numbers/two.wav'
    three = _basepath + 'numbers/three.wav'
    zero = _basepath + 'numbers/zero.wav'
    five = _basepath + 'numbers/five.wav'
    nine = _basepath + 'numbers/nine.wav'
    ready = _basepath + 'system/ready.wav'
    confirm = _basepath + 'system/confirm.wav'
    general_alert = _basepath + 'system/general_alert.wav'
    click = _basepath + 'system/click.wav'
    overpower = _basepath + 'system/overpower.wav'


class Image():
    """Paths to standard EV3 images.

    Information

    .. data:: right
    .. data:: forward
    .. data:: accept
    .. data:: question_mark
    .. data:: stop_1
    .. data:: left
    .. data:: decline
    .. data:: thumbs_down
    .. data:: backward
    .. data:: no_go
    .. data:: warning
    .. data:: stop_2
    .. data:: thumbs_up

    LEGO

    .. data:: ev3
    .. data:: ev3_icon

    Objects

    .. data:: target

    Eyes

    .. data:: bottom_right
    .. data:: bottom_left
    .. data:: evil
    .. data:: crazy_2
    .. data:: knocked_out
    .. data:: pinched_right
    .. data:: winking
    .. data:: dizzy
    .. data:: down
    .. data:: tired_middle
    .. data:: middle_right
    .. data:: sleeping
    .. data:: middle_left
    .. data:: tired_right
    .. data:: pinched_left
    .. data:: pinched_middle
    .. data:: crazy_1
    .. data:: neutral
    .. data:: awake
    .. data:: up
    .. data:: tired_left
    .. data:: angry
    """

    _basepath = '/usr/share/images/ev3dev/mono/'
    right = _basepath + 'information/right.png'
    forward = _basepath + 'information/forward.png'
    accept = _basepath + 'information/accept.png'
    question_mark = _basepath + 'information/question_mark.png'
    stop_1 = _basepath + 'information/stop_1.png'
    left = _basepath + 'information/left.png'
    decline = _basepath + 'information/decline.png'
    thumbs_down = _basepath + 'information/thumbs_down.png'
    backward = _basepath + 'information/backward.png'
    no_go = _basepath + 'information/no_go.png'
    warning = _basepath + 'information/warning.png'
    stop_2 = _basepath + 'information/stop_2.png'
    thumbs_up = _basepath + 'information/thumbs_up.png'
    ev3 = _basepath + 'lego/ev3.png'
    ev3_icon = _basepath + 'lego/ev3_icon.png'
    target = _basepath + 'objects/target.png'
    bottom_right = _basepath + 'eyes/bottom_right.png'
    bottom_left = _basepath + 'eyes/bottom_left.png'
    evil = _basepath + 'eyes/evil.png'
    crazy_2 = _basepath + 'eyes/crazy_2.png'
    knocked_out = _basepath + 'eyes/knocked_out.png'
    pinched_right = _basepath + 'eyes/pinched_right.png'
    winking = _basepath + 'eyes/winking.png'
    dizzy = _basepath + 'eyes/dizzy.png'
    down = _basepath + 'eyes/down.png'
    tired_middle = _basepath + 'eyes/tired_middle.png'
    middle_right = _basepath + 'eyes/middle_right.png'
    sleeping = _basepath + 'eyes/sleeping.png'
    middle_left = _basepath + 'eyes/middle_left.png'
    tired_right = _basepath + 'eyes/tired_right.png'
    pinched_left = _basepath + 'eyes/pinched_left.png'
    pinched_middle = _basepath + 'eyes/pinched_middle.png'
    crazy_1 = _basepath + 'eyes/crazy_1.png'
    neutral = _basepath + 'eyes/neutral.png'
    awake = _basepath + 'eyes/awake.png'
    up = _basepath + 'eyes/up.png'
    tired_left = _basepath + 'eyes/tired_left.png'
    angry = _basepath + 'eyes/angry.png'
