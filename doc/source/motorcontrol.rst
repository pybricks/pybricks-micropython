Motor Essentials
=====================

Intro



Encoded Motors
----------------

.. _duty:

Duty cycle
^^^^^^^^^^
TODO

Angle encoder
^^^^^^^^^^^^^
TODO

.. _ratio:

Gear ratio
^^^^^^^^^^

TODO

        ::

            Example for one gear train: gears=[12, 36]
             ____________
            |            |
            |    motor   |
            |____________|
                  ||
                  || 12t      36t
                ||||||  ||||||||||||||
                              ||
                              ||
                          output axle


            Example with multiple gear trains: gears=[[12, 20, 36], [20, 40], [20, 8, 40]]
            _____________
            |            |
            |    motor   |
            |____________|
                  ||
                  || 12t    20t           36t
                ||||||  |||||||||||  ||||||||||||||
                                           ||
                                           ||
                                        ||||||||  |||||||||||||||||
                                            20t          || 40t
                                                         ||
                                                      ||||||||  |||  |||||||||||||||||
                                                          20t    8t         || 40t
                                                                            ||
                                                                        output axle



Motor maneuvers
---------------


Reference signal
^^^^^^^^^^^^^^^^


Automatic control
^^^^^^^^^^^^^^^^^

.. _pid:

PID Parameters
^^^^^^^^^^^^^^
TODO


.. _defaultpars:

Default motor parameters
------------------------
TODO
