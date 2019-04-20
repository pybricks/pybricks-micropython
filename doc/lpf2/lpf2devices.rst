:mod:`lpf2` -- Power Function 2.0 Motors and Sensors
====================================================

.. automodule:: lpf2devices
    :no-members:

Motors
------

.. autoclass:: lpf2devices.Motor
    :no-members:

    .. rubric:: Methods for motors without rotation sensors

    .. automethod:: lpf2devices.Motor.dc

    .. rubric:: Methods for motors with rotation sensors

    .. automethod:: lpf2devices.Motor.angle

    .. automethod:: lpf2devices.Motor.reset_angle

    .. automethod:: lpf2devices.Motor.speed

    .. automethod:: lpf2devices.Motor.stop

    .. automethod:: lpf2devices.Motor.run

    .. automethod:: lpf2devices.Motor.run_time

    .. automethod:: lpf2devices.Motor.run_angle

    .. automethod:: lpf2devices.Motor.run_target

    .. rubric:: Advanced methods for motors with rotation sensors

    .. automethod:: lpf2devices.Motor.track_target

    .. automethod:: lpf2devices.Motor.stalled

    .. automethod:: lpf2devices.Motor.run_until_stalled

    .. automethod:: lpf2devices.Motor.set_dc_settings

    .. automethod:: lpf2devices.Motor.set_run_settings

    .. automethod:: lpf2devices.Motor.set_pid_settings

Sensors
-------

Color and Distance Sensor
^^^^^^^^^^^^^^^^^^^^^^^^^
.. autoclass:: lpf2devices.ColorDistanceSensor
