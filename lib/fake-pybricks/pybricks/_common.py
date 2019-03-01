"""Generic cross-platform module for typical hub devices like displays, speakers, and batteries."""

from parameters import Align


class Display():
    """Show images or text on a display."""

    def __init__(self, device_type):
        """Device-specific display initialization."""
        pass

    def clear(self):
        """Clear everything on the display."""
        pass

    def text(self, text, coordinate=None):
        """Display text.

        Parameters:
            text (str): The text to display.
            coordinate (tuple): ``(x, y)`` coordinate tuple. It is the top-left corner of the first character. If no coordinate is specified, it is printed on the next line.

        Example::

            # Clear the display
            brick.display.clear()

            # Print ``Hello`` near the middle of the screen
            brick.display.text("Hello", (60, 50))

            # Print ``World`` directly underneath it
            brick.display.text("World")

        """
        pass

    def image(self, file_name, alignment=Align.CENTER, coordinate=None, clear=True):
        """image(file_name, alignment=Align.CENTER, coordinate=None, clear=True)

        Show an image file.

        You can specify its placement either using ``alignment`` or by specifying a ``coordinate``, but not both.

        Arguments:
            file_name (str): Path to the image file. Paths may be absolute or relative from the project folder.
            alignment (Align): Where to place the image (*Default*: Align.CENTER).
            coordinate (tuple): ``(x, y)`` coordinate tuple. It is the top-left corner of the image (*Default*: None).
            clear (bool): Whether to clear the screen before showing the image (*Default*: ``True``).

        Example::

            # Show a built-in image of two eyes looking upward
            brick.display.image(ImageFile.UP)

            # Display a custom image from your project folder
            brick.display.image('pybricks.png')

            # Display a custom image at the top right of the screen, without clearing
            # the screen first
            brick.display.image('arrow.png', Align.TOP_RIGHT, clear=False)
        """
        pass


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
            brick.sound.file(SoundFile.HELLO)

            # Play a sound file from your project folder
            brick.sound.file('mysound.wav')

        """

        pass


class Battery():
    """Get the status of a battery."""

    def __init__(self, device_type):
        """Battery-specific initialization."""
        pass

    def voltage(self):
        """Get the voltage of the battery.

        Returns:
            :ref:`voltage`: Battery voltage.

        Examples::

            # Play a warning sound when the battery voltage
            # is below 7 Volt (7000 mV = 7V)
            if brick.battery.voltage() < 7000:
                brick.sound.beep()

        """
        pass

    def current(self):
        """Get the current supplied by the battery.

        Returns:
            :ref:`current`: Battery current.

        """
        pass
