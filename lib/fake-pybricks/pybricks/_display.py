"""Generic display module for showing text or images on a display (e.g. EV3 screen)."""


from parameters import Align


class Display():
    """Show images or text on a display."""

    def __init__(self, device_type):
        """Device specific display initialization."""
        pass

    def clear(self):
        """Clear everything on the screen."""
        pass

    def text(self, text, location=None):
        """Write text on the screen.

        Parameters:
            text (str): The text to display.
            location (tuple): (x, y) coordinate. If no location is specified, it just continues on the next line.

        """
        pass

    def image(self, file_name, alignment=Align.center, location=None, clear=True):
        """Show an image file on the display.

        You can specify its placement either using ``alignment`` or by specifying an (x, y) ``location``.

        Arguments:
            file_name (str): Path to the image file.
            alignment (Align): Where to place the image. (*Default*: Align.center)
            location (tuple): Where to place the image. (*Default*: None)
            clear (bool): Whether to clear the screen before showing the image. (*Default*: ``True``)
        """
        pass

