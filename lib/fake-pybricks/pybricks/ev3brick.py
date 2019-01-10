"""LEGO MINDSTORMS EV3 Brick"""

from parameters import Color, Pattern
from _speaker import Speaker
from _display import Display

def buttons():
    """Check which buttons of the EV3 brick are currently pressed.

    :returns: List of pressed buttons.
    :rtype: List of :class:`Button <parameters.Button>`

    ::

        if Button.left in brick.buttons():
            print("The left button is pressed.")

    """
    pass


def light(color, pattern=Pattern.solid, brightness=100):
    """light(color, pattern=Pattern.solid, brightness=100)

    Set the color, pattern, and brightness of the brick light.

    Arguments:
        color (:class:`Color <parameters.Color>` or :ref:`colortuple`): Color of the light.
        pattern (:class:`Pattern <parameters.Pattern>`): List of (:ref:`percentage`, :ref:`time`) tuples that give the brightness and duration of each segment of the pattern.
        brightness (:ref:`percentage`): Peak brightness of the selected color or pattern.

    ::

        # Make the light red
        brick.light(Color.red)

        # Make the light yellow and make it glow on and off.
        brick.light(Color.yellow, Pattern.glow)

        # Make the light green with a custom on-off pattern.
        # The light is on at 100% for 250 ms and then off (0%) for 250 ms.
        my_pattern = [(100, 250), (0, 250)]
        brick.light(Color.green, my_pattern)
    """
    pass

sound = Speaker('EV3')
display = Display('EV3')
