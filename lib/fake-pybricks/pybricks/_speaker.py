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

        Example::

            # A simple beep
            brick.sound.beep()

            # A high pitch (1500 Hz) for one second (1000 ms) at 50% volume
            brick.sound.beep(1500, 1000, 50)
        """
        pass

    def beeps(self, number):
        """Play a number of default beeps with a brief pause in between.

        Arguments:
            number (int): Number of beeps.

        Example::

            # Make 5 simple beeps
            brick.sound.beeps(5)
        """
        pass

    def file(self, file_name, volume=100):
        """Play a sound file.

        Arguments:
            file_name (str): Path to the sound file, including extension.
            volume (:ref:`percentage`): Volume of the sound (*Default*: 100).

        Example::

            # Play one of the built-in sounds
            brick.sound.file(Sound.HELLO)

            # Play a sound file from your project folder
            brick.sound.file('mysound.wav')

        """

        pass
