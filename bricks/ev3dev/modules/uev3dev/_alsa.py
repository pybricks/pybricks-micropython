# MIT License
#
# Copyright (c) 2017 ev3dev.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from errno import EPIPE
from struct import calcsize
from struct import unpack

import ffilib
from uctypes import addressof

# from uev3dev.util import debug_print

_alsa = ffilib.open('libasound')
_strerror = _alsa.func('s', 'snd_strerror', 'i')


def _check_error(err):
    if err < 0:
        raise AlsaError(_strerror(err))


class AlsaError(Exception):
    def __init__(self, message):
        super(AlsaError, self).__init__(message)


class Mixer():
    _open = _alsa.func('i', 'snd_mixer_open', 'pi')
    _close = _alsa.func('i', 'snd_mixer_close', 'p')
    _attach = _alsa.func('i', 'snd_mixer_attach', 'ps')
    _load = _alsa.func('i', 'snd_mixer_load', 'p')

    _selem_register = _alsa.func('i', 'snd_mixer_selem_register', 'ppp')
    _selem_register = _alsa.func('i', 'snd_mixer_selem_register', 'ppp')
    _selem_id_sizeof = _alsa.func('p', 'snd_mixer_selem_id_sizeof', '')
    _selem_id_set_index = _alsa.func('v', 'snd_mixer_selem_id_set_index', 'pI')
    _selem_id_set_name = _alsa.func('v', 'snd_mixer_selem_id_set_name', 'ps')
    _find_selem = _alsa.func('p', 'snd_mixer_find_selem', 'pP')
    _selem_get_playback_volume_range = \
        _alsa.func('i', 'snd_mixer_selem_get_playback_volume_range', 'ppp')
    _selem_set_playback_volume_all = \
        _alsa.func('i', 'snd_mixer_selem_set_playback_volume_all', 'pl')

    def __init__(self):
        self._mixer = bytearray(calcsize('P'))
        err = Mixer._open(self._mixer, 0)
        _check_error(err)
        self._mixer = unpack('P', self._mixer)[0]
        try:
            # use default sound card
            err = Mixer._attach(self._mixer, 'default')
            _check_error(err)
            err = Mixer._selem_register(self._mixer, 0, 0)
            _check_error(err)
            err = Mixer._load(self._mixer)
            _check_error(err)

            self._id_data = bytearray(Mixer._selem_id_sizeof())
            self._id = addressof(self._id_data)
            min = bytearray(calcsize('l'))
            max = bytearray(calcsize('l'))

            # get PCM volume control
            Mixer._selem_id_set_index(self._id, 0)
            Mixer._selem_id_set_name(self._id, 'PCM')
            self._pcm_elem = Mixer._find_selem(self._mixer, self._id)
            if not self._pcm_elem:
                raise AlsaError('Could not find "PCM" mixer element')
            Mixer._selem_get_playback_volume_range(self._pcm_elem,
                                                   addressof(min),
                                                   addressof(max))
            self._pcm_min = unpack('l', min)[0]
            self._pcm_max = unpack('l', max)[0]

            # get Beep volume control
            Mixer._selem_id_set_index(self._id, 0)
            Mixer._selem_id_set_name(self._id, 'Beep')
            self._beep_elem = Mixer._find_selem(self._mixer, self._id)
            if not self._beep_elem:
                raise AlsaError('Could not find "Beep" mixer element')
            Mixer._selem_get_playback_volume_range(self._beep_elem, min, max)
            self._beep_min = unpack('l', min)[0]
            self._beep_max = unpack('l', max)[0]
        except:
            Mixer._close(self._mixer)
            raise

    def close(self):
        self._beep_elem = None
        self._pcm_elem = None
        if self._mixer:
            Mixer._close(self._mixer)
            self._mixer = None

    def set_pcm_volume(self, volume):
        # scale the volume, assuming self._pcm_min is 0
        volume = volume * self._pcm_max // 100
        Mixer._selem_set_playback_volume_all(self._pcm_elem, volume)

    def set_beep_volume(self, volume):
        # scale the volume, assuming self._beep_min is 0
        volume = volume * self._beep_max // 100
        Mixer._selem_set_playback_volume_all(self._beep_elem, volume)


class PCM():
    _open = _alsa.func('i', 'snd_pcm_open', 'pPIi')
    _writei = _alsa.func('l', 'snd_pcm_writei', 'ppL')
    _prepare = _alsa.func('l', 'snd_pcm_prepare', 'p')
    _drop = _alsa.func('l', 'snd_pcm_drop', 'p')
    _drain = _alsa.func('l', 'snd_pcm_drain', 'p')
    _close = _alsa.func('l', 'snd_pcm_close', 'p')
    _hw_params = _alsa.func('i', 'snd_pcm_hw_params', 'pp')
    _hw_params_sizeof = _alsa.func('p', 'snd_pcm_hw_params_sizeof', '')
    _hw_params_any = _alsa.func('i', 'snd_pcm_hw_params_any', 'pp')
    _hw_params_set_access = _alsa.func('i', 'snd_pcm_hw_params_set_access', 'ppI')
    _hw_params_set_format = _alsa.func('i', 'snd_pcm_hw_params_set_format', 'ppI')
    _hw_params_set_channels = _alsa.func('i', 'snd_pcm_hw_params_set_channels', 'ppI')
    _hw_params_set_rate = _alsa.func('i', 'snd_pcm_hw_params_set_rate', 'ppIi')
    _hw_params_get_period_size = \
        _alsa.func('i', 'snd_pcm_hw_params_get_period_size', 'ppp')

    _STREAM_PLAYBACK = 0
    _ACCESS_RW_INTERLEAVED = 3
    _FORMAT_S16_LE = 2

    def __init__(self):
        self._pcm = bytearray(calcsize('P'))
        err = PCM._open(self._pcm, 'default', PCM._STREAM_PLAYBACK, 0)
        _check_error(err)
        self._pcm = unpack('P', self._pcm)[0]
        try:
            self._hp = bytearray(PCM._hw_params_sizeof())
            err = PCM._hw_params_any(self._pcm, self._hp)
            _check_error(err)
        except:
            PCM._close(self._pcm)
            raise

    def close(self):
        if self._pcm:
            PCM._close(self._pcm)
            self._pcm = None

    def play(self, sound_file, cancel_token=None):
        err = PCM._hw_params_set_access(self._pcm, self._hp,
                                        PCM._ACCESS_RW_INTERLEAVED)
        _check_error(err)
        err = PCM._hw_params_set_format(self._pcm, self._hp,
                                        PCM._FORMAT_S16_LE)
        _check_error(err)
        err = PCM._hw_params_set_channels(self._pcm, self._hp,
                                          sound_file._channels)
        _check_error(err)
        err = PCM._hw_params_set_rate(self._pcm, self._hp,
                                      sound_file._samplerate, 0)
        _check_error(err)

        err = PCM._hw_params(self._pcm, self._hp)
        _check_error(err)
        frames = bytearray(calcsize('L'))
        direction = bytearray(calcsize('i'))
        err = PCM._hw_params_get_period_size(self._hp, frames, direction)
        _check_error(err)
        frames = unpack('L', frames)[0]

        for buf, count in sound_file._read(frames):
            if cancel_token and cancel_token.canceled:
                PCM._drop(self._pcm)
                _check_error(err)
                return
            err = PCM._writei(self._pcm, buf, count)
            if err == -EPIPE:
                PCM._prepare(self._pcm)
            else:
                _check_error(err)

        err = PCM._drain(self._pcm)
        _check_error(err)
