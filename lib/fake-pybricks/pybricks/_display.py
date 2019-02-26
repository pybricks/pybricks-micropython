"""Generic display module for showing text or images on a display (e.g. EV3 screen)."""


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

    def image(self, file_name, alignment=Align.center, coordinate=None, clear=True):
        """image(file_name, alignment=Align.center, coordinate=None, clear=True)

        Show an image file.

        You can specify its placement either using ``alignment`` or by specifying a ``coordinate``, but not both.

        Arguments:
            file_name (str): Path to the image file. Paths may be absolute or relative from the project folder.
            alignment (Align): Where to place the image (*Default*: Align.center).
            coordinate (tuple): ``(x, y)`` coordinate tuple. It is the top-left corner of the image (*Default*: None).
            clear (bool): Whether to clear the screen before showing the image (*Default*: ``True``).

        Example::

            # Show a built-in image showing two eyes looking upward
            brick.display.image(Image.up)

            # Display a custom image from your project folder
            brick.display.image('pybricks.png')

            # Display a custom image image at the top right of the screen, without clearing
            # the screen first
            brick.display.image('arrow.png', Align.top_right, clear=False)
        """
        pass

