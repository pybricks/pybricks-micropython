# SPDX-License-Identifier: MIT
# Copyright (c) 2017 David Lechner <david@lechnology.com>

import _thread

from struct import calcsize
from struct import pack
from struct import unpack
from time import sleep, sleep_ms

import ffilib
from uctypes import addressof
from uctypes import INT32
from uctypes import sizeof
from uctypes import struct
from uctypes import UINT16
from uctypes import UINT32
from uctypes import UINT64

from ._alsa import Mixer
from ._alsa import PCM
from .util import debug_print
from .util import Timeout

# TODO: os.SEEK_SET is not defined in micropython-lib
_SEEK_SET = 0

_BEEP_DEV = '/dev/input/by-path/platform-sound-event'


# stuff from linux/input.h and linux/input-event-codes.h

_EV_SND = 0x12
_SND_TONE = 0x02
_input_event = {
    'time': UINT64 | 0,  # struct timeval
    'type': UINT16 | 8,
    'code': UINT16 | 10,
    'value': INT32 | 12,
}


# libsndfile

_libsndfile = ffilib.open('libsndfile')

_sf_count_t = UINT64

_SF_INFO = {
    'frames': _sf_count_t | 0,
    'samplerate': INT32 | 8,
    'channels': INT32 | 12,
    'format': INT32 | 16,
    'sections': INT32 | 20,
    'seekable': INT32 | 24,
}

_SMF_READ = 0x10

# FIXME: micropython does not have 64-bit integer, using double for now
_sf_open = _libsndfile.func('p', 'sf_open', 'Pip')
_sf_close = _libsndfile.func('i', 'sf_close', 'p')
_sf_seek = _libsndfile.func('d', 'sf_seek', 'pdi')
_sf_readf_short = _libsndfile.func('d', 'sf_readf_short', 'ppd')
_sf_strerror = _libsndfile.func('s', 'sf_strerror', 'p')


# from lms2012

_NOTES = {
  'C4': 262,
  'D4': 294,
  'E4': 330,
  'F4': 349,
  'G4': 392,
  'A4': 440,
  'B4': 494,
  'C5': 523,
  'D5': 587,
  'E5': 659,
  'F5': 698,
  'G5': 784,
  'A5': 880,
  'B5': 988,
  'C6': 1047,
  'D6': 1175,
  'E6': 1319,
  'F6': 1397,
  'G6': 1568,
  'A6': 1760,
  'B6': 1976,
  'C#4': 277,
  'D#4': 311,
  'F#4': 370,
  'G#4': 415,
  'A#4': 466,
  'C#5': 554,
  'D#5': 622,
  'F#5': 740,
  'G#5': 831,
  'A#5': 932,
  'C#6': 1109,
  'D#6': 1245,
  'F#6': 1480,
  'G#6': 1661,
  'A#6': 1865,
}


class PlayType():
    """List of values for ``play_type`` in sound playback methods"""
    WAIT = 0
    """Play the sound once and wait until it is finished before returning"""
    ONCE = 1
    """Play the sound once in the background"""
    REPEAT = 2
    """Play the sound repeating in the background"""


class Sound():
    """Object for making sounds"""
    def __init__(self):
        self._beep_dev = open(_BEEP_DEV, 'b+')
        self._mixer = Mixer()
        self._pcm = PCM()
        self._tone_data = bytearray(sizeof(_input_event))
        self._tone_event = struct(addressof(self._tone_data), _input_event)
        self._timeout = Timeout(0, None)
        self._cancel = None
        self._lock = _thread.allocate_lock()

    def _play_tone(self, frequency):
        self._tone_event.type = _EV_SND
        self._tone_event.code = _SND_TONE
        self._tone_event.value = int(frequency)
        self._beep_dev.write(self._tone_data)

    def _beep(self, frequency, duration, volume):
        self._mixer.set_beep_volume(int(volume))
        self._play_tone(frequency)
        sleep_ms(int(duration))
        self._play_tone(0)

    def play_tone(self, frequency, duration, volume, play_type):
        """Play a tone

        Parameters:
            frequency (int): The frequency of the tone in Hz
            duration (float): The duration of the tone in seconds
            volume (int): The playback volume in percent [0..100]
            play_type (PlayType): Controls how many times the sound is played
                and when the function returns
        """
        with self._lock:
            self._stop()
            self._timeout._interval = duration
            self._timeout._repeat = play_type == PlayType.REPEAT
            if self._timeout._repeat:
                self._timeout._func = lambda: self._play_tone(frequency)
            else:
                self._timeout._func = lambda: self._play_tone(0)
            self._mixer.set_beep_volume(volume)
            self._play_tone(frequency)
            self._timeout.start()

        if play_type == PlayType.WAIT:
            self._timeout.wait()

    def play_note(self, note, duration, volume, play_type):
        """Play a musical note

        Parameters:
            note (int): The name of the note ['C4'..'B6'] (use # for sharp)
            duration (float): The duration of the note in seconds
            volume (int): The playback volume in percent [0..100]
            play_type (PlayType): Controls how many times the sound is played
                and when the function returns
        """
        frequency = _NOTES[note]
        self.play_tone(frequency, duration, volume, play_type)

    def play_file(self, file, volume, play_type):
        """Play a .WAV file

        Parameters:
            file (str): The file path
            volume (int): The playback volume in percent [0..100]
            play_type (PlayType): Controls how many times the sound is played
                and when the function returns
        """
        with self._lock:
            self._stop()
            self._mixer.set_pcm_volume(volume)
            token = file._cancel_token()

            sync_lock = _thread.allocate_lock()

            def play():
                try:
                    while True:
                        self._pcm.play(file, token)
                        if play_type != PlayType.REPEAT or token.canceled:
                            break
                finally:
                    sync_lock.release()

            sync_lock.acquire()
            _thread.start_new_thread(play, ())
            self._cancel = token

            if play_type == PlayType.WAIT:
                with sync_lock:
                    pass

    def _stop(self):
        self._timeout.cancel()
        if self._cancel:
            self._cancel.cancel()
            self._cancel = None
        self._play_tone(0)

    def stop(self):
        """Stop any sound that is playing"""
        with self._lock:
            self._stop()


class SoundFileError(Exception):
    def __init__(self, message):
        super(SoundFileError, self).__init__(message)


class SoundFile():
    """Class that represents a sound file.

    Parameters:
        path (str): The path to a sound file
    """
    def __init__(self, path):
        info_data = bytearray(sizeof(_SF_INFO))
        info = struct(addressof(info_data), _SF_INFO)
        self._file = _sf_open(path, _SMF_READ, info_data)
        if not self._file:
            raise SoundFileError(_sf_strerror(0))
        self._frames = info.frames
        self._samplerate = info.samplerate
        self._channels = info.channels
        self._format = info.format
        self._sections = info.sections
        self._seekable = bool(info.seekable)

    def close(self):
        if self._file:
            _sf_close(self._file)
            self._file = None

    def _read(self, frames, cancel_token=None):
        if not self._file:
            raise RuntimeError('SoundFile has been closed')
        frames_ = unpack('d', pack('Q', 0))[0]
        _sf_seek(self._file, frames_, _SEEK_SET)
        buf = bytearray(frames * self._channels * calcsize('h'))
        while True:
            if cancel_token and cancel_token.canceled:
                break
            # FIXME: micropython ffi doesn't have 64-bit integer type
            # double is the only 64-bit type, so using a hack to make it work
            frames_ = unpack('d', pack('Q', frames))[0]
            readcount_ = _sf_readf_short(self._file, buf, frames_)
            readcount = unpack('Q', pack('d', readcount_))[0]
            if not readcount:
                break
            yield buf, readcount

    def _cancel_token(self):
        return _CancelToken()


class _CancelToken():
    def __init__(self):
        self.canceled = False

    def cancel(self):
        self.canceled = True
