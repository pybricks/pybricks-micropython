"""LEGO MINDSTORMS EV3 Brick"""

from parameters import Color
from _speaker import Speaker
from _display import Display

def buttons():
    """Check which buttons of the EV3 brick are currently pressed.

    :returns: List of pressed buttons.
    :rtype: List of :class:`Button <parameters.Button>`

    Example::

        if Button.left in brick.buttons():
            print("The left button is pressed.")

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
