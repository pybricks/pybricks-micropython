Signals and Units
=================

Many commands allow you to specify arguments in terms of well-known physical quantities. This page gives an overview of each quantity and its unit.

.. _time:

time: ms
---------
All time and duration values are measured in milliseconds (ms).

For example, the duration of motion with :meth:`run_time <.ev3devices.Motor.run_time>`, the duration of :func:`wait <.tools.wait>`, or the time values returned by the :class:`StopWatch <.tools.StopWatch>` are specified in milliseconds.

.. _angle:

angle: deg
-----------
All angles are measured in degrees (deg). One full rotation corresponds to 360 degrees.

For example, the angle values of a :meth:`Motor <.ev3devices.Motor.angle>` or the :meth:`GyroSensor <.ev3devices.GyroSensor.angle>` are expressed in degrees.

.. _speed:

rotational speed: deg/s
-----------------------

Rotational speed, or *angular velocity* describes how fast something rotates, expressed as the number of degrees per second (deg/s).

For example, the rotational speed values of a :meth:`Motor <.ev3devices.Motor.speed>` or the :meth:`GyroSensor <.ev3devices.GyroSensor.speed>` are expressed in degrees per second.

While we recommend working with degrees per second in your programs, you can use the following table to convert between commonly used units.

+-----------+-------+-----------+
|           | deg/s | rpm       |
+-----------+-------+-----------+
| 1 deg/s = | 1     | 1/6=0.167 |
+-----------+-------+-----------+
| 1 rpm =   | 6     | 1         |
+-----------+-------+-----------+

.. _distance:

distance: mm
-------------
Distances are expressed in millimeters (mm) whenever possible.

For example, the distance value of the :meth:`UltrasonicSensor <.ev3devices.UltrasonicSensor.distance>` is measured in millimeters.

While we recommend working with millimeters in your programs, you can use the following table to convert between commonly used units.

+---------+------+-----+--------+
|         | mm   | cm  | inch   |
+---------+------+-----+--------+
| 1 mm =  | 1    | 0.1 | 0.0394 |
+---------+------+-----+--------+
| 1 cm =  | 10   | 1   | 0.394  |
+---------+------+-----+--------+
| 1 inch =| 25.4 | 2.54| 1      |
+---------+------+-----+--------+

.. _dimension:

dimension: mm
-------------
Dimensions are expressed in millimeters (mm) whenever possible, just like distances.

For example, the diameter of a wheel is measured in millimeters.


.. _relativedistance:

relative distance: %
---------------------

Some distance measurements do not provide an accurate value with a specific unit, but they range from very close (0%) to very far (100%). These are referred to as relative distances.

For example, the distance value of the :meth:`InfraredSensor <.ev3devices.InfraredSensor.distance>` is a relative distance.




.. _travelspeed:

speed: mm/s
------------
Linear speeds are expressed as millimeters per second (mm/s).

For example, the speed of a robotic vehicle is expressed in mm/s.

.. _acceleration:

rotational acceleration: deg/s/s
--------------------------------
Rotational acceleration, or *angular acceleration* describes how fast the rotational speed changes. This is expressed as the change of the number of degrees per second, during one second (deg/s/s). This is also commonly written as  :math:`deg/s^2`.

For example, you can adjust the rotational acceleration setting of a :meth:`Motor <.ev3devices.Motor.set_run_settings>` to change how smoothly or how quickly it reaches the constant speed set point.


.. _percentage:

percentage: %
--------------
Some signals do not have specific units but range from a minimum (0%) to a maximum (100%). A specific type of percentages are :ref:`relative distances <relativedistance>`.

For example, the sound :meth:`volume <.ev3brick.sound.beep>` ranges from 0% to 100%.

.. _frequency:

frequency: Hz
--------------
Sound frequencies are expressed in Hertz (Hz).

For example, you can choose the frequency of a :meth:`beep <.ev3brick.sound.beep>` to change the pitch.

.. _voltage:

voltage: mV
--------------
Voltages are expressed in millivolt (mV).

For example, you can check the voltage of the :meth:`battery <.ev3brick.battery.voltage>`.

.. _current:

current: mA
--------------
Electrical currents are expressed in milliampere (mA).

For example, you can check the current supplied by the :meth:`battery <.ev3brick.battery.current>`.
