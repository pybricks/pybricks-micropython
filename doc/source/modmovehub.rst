:mod:`movehub` -- The LEGO BOOST Move Hub
=========================================

The LEGO BOOST Move Hub is the "brain" of a robotics kit released in 2017 aimed
at younger children.


The Ports
---------

The Move Hub has two I/O ports labeled 'C' and 'D' for connecting sensors and
motors. There are also two built-in motors that are connected to virtual ports
'A' and 'B'. The hub itself is also a virtual port for the button, light and
internal tilt sensor.

.. autoclass:: movehub.Port


The Button
----------

There is one button on the Move Hub. 

.. autodata:: movehub.button
.. FIXME: for some reason this is picking up the doc from the class instead of the doc comment



The Light
---------

The light on the Move Hub is an RGB LED that can display just about any color.

.. autodata:: movehub.light
.. FIXME: for some reason this is picking up the doc from the class instead of the doc comment


The Tilt Sensor
---------------

The Move Hub has a built-in accelerometer.

.. todo:: Nothing implemented for this yet


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

