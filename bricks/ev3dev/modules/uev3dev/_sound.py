# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
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

from uerrno import ENODEV
from uctypes import addressof, sizeof, struct
from uctypes import INT32, UINT16, UINT64
from utime import sleep_ms

from uev3dev._alsa import Mixer
from uev3dev._alsa import PCM

# FIXME: this path is only for ev3dev-stretch on LEGO MINDSTORMS EV3
_BEEP_DEV = '/dev/input/by-path/platform-sound-event'
_EV_SND = 0x12
_SND_TONE = 0x02
_input_event = {
    'time': UINT64 | 0,
    'type': UINT16 | 8,
    'code': UINT16 | 10,
    'value': INT32 | 12,
}

try:
    _beep_dev = open(_BEEP_DEV, 'b+')
    _mixer = Mixer()
    _pcm = PCM()
    _tone_data = bytearray(sizeof(_input_event))
    _tone_event = struct(addressof(_tone_data), _input_event)

    def _beep(frequency, duration, volume):
        _mixer.set_beep_volume(int(volume))
        _tone_event.type = _EV_SND
        _tone_event.code = _SND_TONE
        _tone_event.value = int(frequency)
        _beep_dev.write(_tone_data)
        sleep_ms(int(duration))
        _tone_event.value = 0
        _beep_dev.write(_tone_data)

except OSError:
    def _beep(frequency, duration, volume):
        raise OSError(ENODEV)
