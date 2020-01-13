from pybricks.hubs import EV3Brick
from pybricks.media.ev3dev import SoundFile

ev3 = EV3Brick()


# beep method

# beep with no args is OK
ev3.speaker.beep()

# beep with 1 arg is OK
ev3.speaker.beep(500)

# beep with 2 args is OK
ev3.speaker.beep(500, 500)

# beep with keyword args is OK
ev3.speaker.beep(frequency=500, duration=500)

try:
    # beep with 3 args is not OK
    ev3.speaker.beep(500, 500, 500)
except TypeError as ex:
    print(ex)

# floats are converted to int
ev3.speaker.beep(500.1, 500.1)


# beep play_notes method

# Requires 1 arg
try:
    ev3.speaker.play_notes()
except TypeError as ex:
    print(ex)

# one arg is OK
ev3.speaker.play_notes([])

# two args is OK
ev3.speaker.play_notes([], 120)

# keyword args are OK
ev3.speaker.play_notes(notes=[], tempo=120)

# String doesn't work because it iterates each character
try:
    ev3.speaker.play_notes('C4/4')
except ValueError as ex:
    print(ex)

# First character must be A-G or R
try:
    ev3.speaker.play_notes(['X'])
except ValueError as ex:
    print(ex)

# Second character must be 2-8
try:
    ev3.speaker.play_notes(['C1'])
except ValueError as ex:
    print(ex)

# Certain notes can't have sharp or flat
try:
    ev3.speaker.play_notes(['Cb'])
except ValueError as ex:
    print(ex)

# '/' delimiter is required
try:
    ev3.speaker.play_notes(['C#4'])
except ValueError as ex:
    print(ex)

# fraction is required
try:
    ev3.speaker.play_notes(['Db4/'])
except ValueError as ex:
    print(ex)


def notes():
    yield 'E4/4'
    raise RuntimeError("notes iter error")


# exception from iterator propagates
try:
    ev3.speaker.play_notes(notes())
except RuntimeError as ex:
    print(ex)


# play_file method

# Requires one argument
try:
    ev3.speaker.play_file()
except TypeError as ex:
    print(ex)

# one argument OK
ev3.speaker.play_file(SoundFile.HELLO)

# keyword argument OK
ev3.speaker.play_file(file=SoundFile.HELLO)

# file not found gives RuntimeError
try:
    ev3.speaker.play_file('bad')
except RuntimeError as ex:
    print(ex)


# say method

# Requires one argument
try:
    ev3.speaker.say()
except TypeError as ex:
    print(ex)

# one argument OK
ev3.speaker.say('hi')

# keyword argument OK
ev3.speaker.say(text='hi')


# set_volume method

# Requires one argument
try:
    ev3.speaker.set_volume()
except TypeError as ex:
    print(ex)

# one argument OK
ev3.speaker.set_volume(0)

# two arguments OK
ev3.speaker.set_volume(0, 'Beep')

# keyword argument OK
ev3.speaker.set_volume(volume=0, which='Beep')

# only certain values allowed for which=
try:
    ev3.speaker.set_volume(0, 'bad')
except ValueError as ex:
    print(ex)


# set_speech_options method

# No required arguments (although this is noop)
ev3.speaker.set_speech_options()

# one argument is OK
ev3.speaker.set_speech_options('en')

# two options are OK
ev3.speaker.set_speech_options('en', 100)

# three options are OK
ev3.speaker.set_speech_options('en', 100, 50)

# keyword args are OK
ev3.speaker.set_speech_options(voice='en', speed=100, pitch=50)
