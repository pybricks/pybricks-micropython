:mod:`movehub` -- functions related to the LEGO BOOST Move Hub
==============================================================

Button functions
----------------

.. function:: get_button()

    Returns ``True`` if the button is pressed, otherwise ``False``.

LED functions
-------------

.. function:: set_led(color)

    Sets the LED color. The *color* can be the name of a color (``"red"``,
    ``"green"``, ``"blue"``, ``"yellow"``, ``"cyan"``, ``"magenta"``,
    ``"white"``) or ``None`` to turn the LED off. It can also be a tuple with
    and RGB value, e.g. ``(255, 128, 0)``.


Battery functions
-----------------

.. function:: get_bat_mV()

    Returns the battery voltage in mV.

.. function:: get_bat_mA()

    Returns the battery current in mA.


Power and Reset functions
-------------------------

.. function:: reset(fw_update)

   Performs a hard reset. If *fw_update* is ``True``, then the firmware loader
   will run, otherwise the regular firmware will run.

.. function:: power_off()

    Turns the Move Hub off.

