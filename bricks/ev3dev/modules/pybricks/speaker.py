# SPDX-License-Identifier: MIT
# Copyright (c) 2017 Laurens Valk

"""Pybricks sound/speaker module for playing sounds (e.g. EV3 speaker, Bluetooth speaker, or IDE speaker)"""
# TODO: This module should go elsewhere since it isn't EV3 specific.

from .uev3dev.sound import Sound as Ev3devSpeaker
from .uev3dev.sound import SoundFile
from os import path
from .tools import wait


class Speaker():
    """Play beeps and sound files using a speaker."""
    _valid_devices = ['EV3']

    def __init__(self, device_type):
        """Device specific speaker initialization."""
        assert device_type in self._valid_devices, 'Selected device is not supported.'
        if device_type == 'EV3':
            self._device = Ev3devSpeaker()
        self._loaded_files = {}

    def beep(self, frequency=500, duration=100, volume=30):
        """Play a beep.

        Keyword Arguments:
            frequency {int} -- Frequency of the beep (Hz) (default: {500})
            duration {int} -- Duration of the beep (milliseconds) (default: {100})
            volume {int} -- Volume of the beep (0-100%) (default: {30})
        """
        self._device._beep(frequency, duration, volume)

    def beeps(self, number):
        """Play a number of beeps with a brief pause in between.

        Arguments:
            number {int} -- Number of beeps
        """
        for i in range(number):
            self.beep()
            wait(100)

    def tune(self, frequencies_and_durations, volume=30):
        """Play a tune composed of beeps.

        Keyword Arguments:
            frequencies_and_durations {list} -- List of (frequency, duration) pairs
            volume {int} -- Volume of the tune (0-100%) (default: {30})
        """
        for (frequency, duration) in frequencies_and_durations:
            self._device._beep(frequency, duration, volume)

    def file(self, file_name, volume=30):
        """Play a sound file.

        Keyword Arguments:
            file_name {str} -- Path to the sound file
            volume {int} -- Volume of the sound (0-100%) (default: {30})
        """

        if file_name not in self._loaded_files:
            self._loaded_files[file_name] = SoundFile(file_name)
        self._device.play_file(self._loaded_files[file_name], volume, 0)

    def speech(self, text, volume=30):
        """Text-to-speech for the given text.

        Keyword Arguments:
            text {str} -- The text
            volume {int} -- Volume of the sound (0-100%) (default: {30})
        """
        # TODO:
        pass
