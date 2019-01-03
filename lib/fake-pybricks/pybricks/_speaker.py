"""Generic speaker module for playing sounds (e.g. EV3 speaker, Bluetooth speaker, or IDE speaker)."""


class Speaker():
    """Play beeps and sound files using a speaker."""

    def __init__(self, device_type):
        """Device specific speaker initialization."""
        pass

    def beep(self, frequency=500, duration=100, volume=30):
        """Play a beep/tone.

        Arguments:
            frequency (:ref:`frequency`): Frequency of the beep (*Default*: 500).
            duration (:ref:`time`): Duration of the beep (*Default*: 100).
            volume (:ref:`percentage`): Volume of the beep (*Default*: 30).
        """
        pass

    def beeps(self, number):
        """Play a number of default beeps with a brief pause in between.

        Arguments:
            number (int): Number of beeps
        """
        pass

    def tune(self, frequencies_and_durations, volume=30):
        """Play a tune composed of beeps.

        Arguments:
            frequencies_and_durations (list): List of (:ref:`frequency`, :ref:`time`) pairs
            volume (:ref:`percentage`): Volume of the tune (*Default*: 30).
        """
        pass

    def file(self, file_name, volume=100):
        """Play a sound file.

        Arguments:
            file_name (str): Path to the sound file, including extension.
            volume (:ref:`percentage`): Volume of the sound (*Default*: 100).
        """

        pass

    def speak(self, text, volume=30):
        """Speak a given string of text.

        Arguments:
            text (str): The text to speak.
            volume (:ref:`percentage`): Volume of the sound (*Default*: 100).
        """
        pass
