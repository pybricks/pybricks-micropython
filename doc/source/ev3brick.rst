:mod:`ev3brick` -- The EV3 Programmable Brick
=============================================

.. automodule:: ev3brick
    :no-members:


Buttons
-------

.. autofunction:: ev3brick.buttons


Light
-----

.. autofunction:: ev3brick.light



Sound
-----

.. automethod:: ev3brick.sound.beep


.. automethod:: ev3brick.sound.beeps


.. automethod:: ev3brick.sound.file


Display
-------
::

                       x
              -------------->
     (0, 0)  __________________
            |                  |
        |   |                  |
     y  |   |      Hello       |
        |   |      World       |
        v   |                  |
            |__________________|
                                (177, 127)

.. automethod:: ev3brick.display.clear

.. automethod:: ev3brick.display.text

.. automethod:: ev3brick.display.image

Battery
-------

.. automethod:: ev3brick.battery.voltage

.. automethod:: ev3brick.battery.current
