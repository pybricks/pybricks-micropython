:mod:`ev3devices` -- EV3 Motors and Sensors
===========================================

.. automodule:: ev3devices
    :no-members:

Motors
------

.. autoclass:: ev3devices.Motor
    :no-members:

    .. rubric:: Methods for motors without rotation sensors

    .. automethod:: ev3devices.Motor.dc

    .. rubric:: Methods for motors with rotation sensors

    .. automethod:: ev3devices.Motor.angle

    .. automethod:: ev3devices.Motor.reset_angle

    .. automethod:: ev3devices.Motor.speed

    .. automethod:: ev3devices.Motor.stop

    .. automethod:: ev3devices.Motor.run

    .. automethod:: ev3devices.Motor.run_time

    .. automethod:: ev3devices.Motor.run_angle

    .. automethod:: ev3devices.Motor.run_target

    .. rubric:: Advanced methods for motors with rotation sensors

    .. automethod:: ev3devices.Motor.track_target

    .. automethod:: ev3devices.Motor.stalled

    .. automethod:: ev3devices.Motor.run_until_stalled

    .. automethod:: ev3devices.Motor.set_dc_settings

    .. automethod:: ev3devices.Motor.set_run_settings

    .. automethod:: ev3devices.Motor.set_pid_settings

Sensors
-------

Touch Sensor
^^^^^^^^^^^^
.. autoclass:: ev3devices.TouchSensor

Color Sensor
^^^^^^^^^^^^
.. autoclass:: ev3devices.ColorSensor

Infrared Sensor and Beacon
^^^^^^^^^^^^^^^^^^^^^^^^^^
.. autoclass:: ev3devices.InfraredSensor

Ultrasonic Sensor
^^^^^^^^^^^^^^^^^
.. autoclass:: ev3devices.UltrasonicSensor

Gyroscopic Sensor
^^^^^^^^^^^^^^^^^
.. autoclass:: ev3devices.GyroSensor
