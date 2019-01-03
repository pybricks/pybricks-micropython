"""LEGO MINDSTORMS EV3 Brick"""

from parameters import Color, Pattern
from _speaker import Speaker
from _display import Display

def buttons():
    """Check which buttons of the EV3 brick are currently pressed.

    :returns: List of pressed buttons.
    :rtype: List of :class:`Button <parameters.Button>`
    """
    pass


def light(color, pattern=Pattern.solid, brightness=100):
    """light(color, pattern=Pattern.solid, brightness=100)

    Set the color, pattern, and brightness of the brick light.

    Arguments:
        color (:class:`Color <parameters.Color>` or RGB tuple): Color of the light. TODO: consider making the Color "enum" itself RGB tuples...
        pattern (:class:`Pattern <parameters.Color>`): List of (brightness, duration) tuples.
        brightness (:ref:`percentage`): Brightness of the light (0-100).
    """
    pass

sound = Speaker('EV3')
display = Display('EV3')
