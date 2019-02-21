"""LEGO\ :sup:`®` MINDSTORMS\ :sup:`®` EV3 brick."""

from parameters import Color
from _speaker import Speaker
from _display import Display

def buttons():
    """Check which buttons on the EV3 brick are currently pressed.

    :returns: List of pressed buttons.
    :rtype: List of :class:`Button <parameters.Button>`

    Examples::

        # Do something if the left button is pressed
        if Button.LEFT in brick.buttons():
            print("The left button is pressed.")

    ::

        # Wait until any of the buttons is pressed
        while not any(brick.buttons()):
            wait(10)

        # Wait until all buttons are released
        while any(brick.buttons()):
            wait(10)

    """
    pass


def light(color):
    """Set the color of the brick light.

    Arguments:
        color (Color): Color of the light. Choose ``Color.black`` or ``None`` to turn the light off.

    Example::

        # Make the light red
        brick.light(Color.red)

        # Turn the light off
        brick.light(None)
    """
    pass

sound = Speaker('EV3')
display = Display('EV3')
