# The MIT License (MIT)
#
# Copyright (c) 2018 Laurens Valk
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
"""Enum classes of parameters, used by modules in the Pybricks package."""

from parameters_c import Port, Direction, Stop, Color, Button


class Align():
    bottom_left = 1
    bottom = 2
    bottom_right = 3
    left = 4
    center = 5
    right = 6
    top_left = 7
    top = 8
    top_right = 9


class Sound():
    """Paths to standard EV3 sounds."""

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
    """Paths to standard EV3 images."""

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
