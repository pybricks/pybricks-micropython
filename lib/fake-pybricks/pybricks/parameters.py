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

    .. data:: SHOUTING
    .. data:: CHEERING
    .. data:: CRYING
    .. data:: OUCH
    .. data:: LAUGHING_2
    .. data:: SNEEZING
    .. data:: SMACK
    .. data:: BOING
    .. data:: BOO
    .. data:: UH_OH
    .. data:: SNORING
    .. data:: KUNG_FU
    .. data:: FANFARE
    .. data:: CRUNCHING
    .. data:: MAGIC_WAND
    .. data:: LAUGHING_1

    Information

    .. data:: LEFT
    .. data:: BACKWARDS
    .. data:: RIGHT
    .. data:: OBJECT
    .. data:: COLOR
    .. data:: FLASHING
    .. data:: ERROR
    .. data:: ERROR_ALARM
    .. data:: DOWN
    .. data:: FORWARD
    .. data:: ACTIVATE
    .. data:: SEARCHING
    .. data:: TOUCH
    .. data:: UP
    .. data:: ANALYZE
    .. data:: STOP
    .. data:: DETECTED
    .. data:: TURN
    .. data:: START

    Communication

    .. data:: MORNING
    .. data:: EV3
    .. data:: GO
    .. data:: GOOD_JOB
    .. data:: OKEY_DOKEY
    .. data:: GOOD
    .. data:: NO
    .. data:: THANK_YOU
    .. data:: YES
    .. data:: GAME_OVER
    .. data:: OKAY
    .. data:: SORRY
    .. data:: BRAVO
    .. data:: GOODBYE
    .. data:: HI
    .. data:: HELLO
    .. data:: MINDSTORMS
    .. data:: LEGO
    .. data:: FANTASTIC

    Movements

    .. data:: SPEED_IDLE
    .. data:: SPEED_DOWN
    .. data:: SPEED_UP

    Color

    .. data:: BROWN
    .. data:: GREEN
    .. data:: BLACK
    .. data:: WHITE
    .. data:: RED
    .. data:: BLUE
    .. data:: YELLOW

    Mechanical

    .. data:: TICK_TACK
    .. data:: HORN_1
    .. data:: BACKING_ALERT
    .. data:: MOTOR_IDLE
    .. data:: AIR_RELEASE
    .. data:: AIRBRAKE
    .. data:: RATCHET
    .. data:: MOTOR_STOP
    .. data:: HORN_2
    .. data:: LASER
    .. data:: SONAR
    .. data:: MOTOR_START

    Animals

    .. data:: INSECT_BUZZ_2
    .. data:: ELEPHANT_CALL
    .. data:: SNAKE_HISS
    .. data:: DOG_BARK_2
    .. data:: DOG_WHINE
    .. data:: INSECT_BUZZ_1
    .. data:: DOG_SNIFF
    .. data:: T_REX_ROAR
    .. data:: INSECT_CHIRP
    .. data:: DOG_GROWL
    .. data:: SNAKE_RATTLE
    .. data:: DOG_BARK_1
    .. data:: CAT_PURR

    Numbers

    .. data:: ZERO
    .. data:: ONE
    .. data:: TWO
    .. data:: THREE
    .. data:: FOUR
    .. data:: FIVE
    .. data:: SIX
    .. data:: SEVEN
    .. data:: EIGHT
    .. data:: NINE
    .. data:: TEN

    System

    .. data:: READY
    .. data:: CONFIRM
    .. data:: GENERAL_ALERT
    .. data:: CLICK
    .. data:: OVERPOWER

    """

    _BASE_PATH = '/usr/share/sounds/ev3dev/'
    SHOUTING = _BASE_PATH + 'expressions/shouting.wav'
    CHEERING = _BASE_PATH + 'expressions/cheering.wav'
    CRYING = _BASE_PATH + 'expressions/crying.wav'
    OUCH = _BASE_PATH + 'expressions/ouch.wav'
    LAUGHING_2 = _BASE_PATH + 'expressions/laughing_2.wav'
    SNEEZING = _BASE_PATH + 'expressions/sneezing.wav'
    SMACK = _BASE_PATH + 'expressions/smack.wav'
    BOING = _BASE_PATH + 'expressions/boing.wav'
    BOO = _BASE_PATH + 'expressions/boo.wav'
    UH_OH = _BASE_PATH + 'expressions/uh-oh.wav'
    SNORING = _BASE_PATH + 'expressions/snoring.wav'
    KUNG_FU = _BASE_PATH + 'expressions/kung_fu.wav'
    FANFARE = _BASE_PATH + 'expressions/fanfare.wav'
    CRUNCHING = _BASE_PATH + 'expressions/crunching.wav'
    MAGIC_WAND = _BASE_PATH + 'expressions/magic_wand.wav'
    LAUGHING_1 = _BASE_PATH + 'expressions/laughing_1.wav'
    LEFT = _BASE_PATH + 'information/left.wav'
    BACKWARDS = _BASE_PATH + 'information/backwards.wav'
    RIGHT = _BASE_PATH + 'information/right.wav'
    OBJECT = _BASE_PATH + 'information/object.wav'
    COLOR = _BASE_PATH + 'information/color.wav'
    FLASHING = _BASE_PATH + 'information/flashing.wav'
    ERROR = _BASE_PATH + 'information/error.wav'
    ERROR_ALARM = _BASE_PATH + 'information/error_alarm.wav'
    DOWN = _BASE_PATH + 'information/down.wav'
    FORWARD = _BASE_PATH + 'information/forward.wav'
    ACTIVATE = _BASE_PATH + 'information/activate.wav'
    SEARCHING = _BASE_PATH + 'information/searching.wav'
    TOUCH = _BASE_PATH + 'information/touch.wav'
    UP = _BASE_PATH + 'information/up.wav'
    ANALYZE = _BASE_PATH + 'information/analyze.wav'
    STOP = _BASE_PATH + 'information/stop.wav'
    DETECTED = _BASE_PATH + 'information/detected.wav'
    TURN = _BASE_PATH + 'information/turn.wav'
    START = _BASE_PATH + 'information/start.wav'
    MORNING = _BASE_PATH + 'communication/morning.wav'
    EV3 = _BASE_PATH + 'communication/ev3.wav'
    GO = _BASE_PATH + 'communication/go.wav'
    GOOD_JOB = _BASE_PATH + 'communication/good_job.wav'
    OKEY_DOKEY = _BASE_PATH + 'communication/okey-dokey.wav'
    GOOD = _BASE_PATH + 'communication/good.wav'
    NO = _BASE_PATH + 'communication/no.wav'
    THANK_YOU = _BASE_PATH + 'communication/thank_you.wav'
    YES = _BASE_PATH + 'communication/yes.wav'
    GAME_OVER = _BASE_PATH + 'communication/game_over.wav'
    OKAY = _BASE_PATH + 'communication/okay.wav'
    SORRY = _BASE_PATH + 'communication/sorry.wav'
    BRAVO = _BASE_PATH + 'communication/bravo.wav'
    GOODBYE = _BASE_PATH + 'communication/goodbye.wav'
    HI = _BASE_PATH + 'communication/hi.wav'
    HELLO = _BASE_PATH + 'communication/hello.wav'
    MINDSTORMS = _BASE_PATH + 'communication/mindstorms.wav'
    LEGO = _BASE_PATH + 'communication/lego.wav'
    FANTASTIC = _BASE_PATH + 'communication/fantastic.wav'
    SPEED_IDLE = _BASE_PATH + 'movements/speed_idle.wav'
    SPEED_DOWN = _BASE_PATH + 'movements/speed_down.wav'
    SPEED_UP = _BASE_PATH + 'movements/speed_up.wav'
    BROWN = _BASE_PATH + 'colors/brown.wav'
    GREEN = _BASE_PATH + 'colors/green.wav'
    BLACK = _BASE_PATH + 'colors/black.wav'
    WHITE = _BASE_PATH + 'colors/white.wav'
    RED = _BASE_PATH + 'colors/red.wav'
    BLUE = _BASE_PATH + 'colors/blue.wav'
    YELLOW = _BASE_PATH + 'colors/yellow.wav'
    TICK_TACK = _BASE_PATH + 'mechanical/tick_tack.wav'
    HORN_1 = _BASE_PATH + 'mechanical/horn_1.wav'
    BACKING_ALERT = _BASE_PATH + 'mechanical/backing_alert.wav'
    MOTOR_IDLE = _BASE_PATH + 'mechanical/motor_idle.wav'
    AIR_RELEASE = _BASE_PATH + 'mechanical/air_release.wav'
    AIRBRAKE = _BASE_PATH + 'mechanical/airbrake.wav'
    RATCHET = _BASE_PATH + 'mechanical/ratchet.wav'
    MOTOR_STOP = _BASE_PATH + 'mechanical/motor_stop.wav'
    HORN_2 = _BASE_PATH + 'mechanical/horn_2.wav'
    LASER = _BASE_PATH + 'mechanical/laser.wav'
    SONAR = _BASE_PATH + 'mechanical/sonar.wav'
    MOTOR_START = _BASE_PATH + 'mechanical/motor_start.wav'
    INSECT_BUZZ_2 = _BASE_PATH + 'animals/insect_buzz_2.wav'
    ELEPHANT_CALL = _BASE_PATH + 'animals/elephant_call.wav'
    SNAKE_HISS = _BASE_PATH + 'animals/snake_hiss.wav'
    DOG_BARK_2 = _BASE_PATH + 'animals/dog_bark_2.wav'
    DOG_WHINE = _BASE_PATH + 'animals/dog_whine.wav'
    INSECT_BUZZ_1 = _BASE_PATH + 'animals/insect_buzz_1.wav'
    DOG_SNIFF = _BASE_PATH + 'animals/dog_sniff.wav'
    T_REX_ROAR = _BASE_PATH + 'animals/t-rex_roar.wav'
    INSECT_CHIRP = _BASE_PATH + 'animals/insect_chirp.wav'
    DOG_GROWL = _BASE_PATH + 'animals/dog_growl.wav'
    SNAKE_RATTLE = _BASE_PATH + 'animals/snake_rattle.wav'
    DOG_BARK_1 = _BASE_PATH + 'animals/dog_bark_1.wav'
    CAT_PURR = _BASE_PATH + 'animals/cat_purr.wav'
    EIGHT = _BASE_PATH + 'numbers/eight.wav'
    SEVEN = _BASE_PATH + 'numbers/seven.wav'
    SIX = _BASE_PATH + 'numbers/six.wav'
    FOUR = _BASE_PATH + 'numbers/four.wav'
    TEN = _BASE_PATH + 'numbers/ten.wav'
    ONE = _BASE_PATH + 'numbers/one.wav'
    TWO = _BASE_PATH + 'numbers/two.wav'
    THREE = _BASE_PATH + 'numbers/three.wav'
    ZERO = _BASE_PATH + 'numbers/zero.wav'
    FIVE = _BASE_PATH + 'numbers/five.wav'
    NINE = _BASE_PATH + 'numbers/nine.wav'
    READY = _BASE_PATH + 'system/ready.wav'
    CONFIRM = _BASE_PATH + 'system/confirm.wav'
    GENERAL_ALERT = _BASE_PATH + 'system/general_alert.wav'
    CLICK = _BASE_PATH + 'system/click.wav'
    OVERPOWER = _BASE_PATH + 'system/overpower.wav'


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
