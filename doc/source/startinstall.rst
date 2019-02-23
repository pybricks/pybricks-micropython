Installation
===================

What you need: system requirements
-----------------------------------------------------------
For running MicroPython on the EV3 Intelligent Brick you need at least:
- A Windows 10 or Mac OS computer that is connected to the internet. The internet connection is only needed during the installation. It is not required for programming the brick.
- Administrator acces to the computer for flashing the micro SD card. Administrator access is not required to for programming.
- A micro SD card. You'll need a card with a minimum size of 4GB and a maximum size of 32GB, also known as microSDHC cards. We recommend cards with Application Performance Class A1.
- A micro SD card slot or card reader in your computer.
- A mini-USB cable, like the one included included with your EV3 set.

The typical configuration for developing a MicroPython program for your EV3 robot is shown in the figure below.

.. figure:: images/overview.png
   :width: 90 %
   :alt: overview
   :align: center

   Overview of the required hardware


Setting up a Python develpment environment on your computer
-----------------------------------------------------------

The easiest way to write your MicroPython programs is using Microsoft Visual Studio Code (VS Code). It is a free and powerful development environment. We have developed extensions to facilitate communication with the EV3 Brick and setting up new projects. Follow the steps below to download, install and configure VS Code:

1. Download `Visual Studio Code <https://code.visualstudio.com/Download>`_.
2. Follow the on-screen instructions to install the application.
3. Launch Visual Studio Code
4. Open the extensions tab as shown below.
4. Install and activate the required extensions:

.. warning::

    The all-in-one extension is not yet available in the Visual Studio Code marketplace.
    
    Please install the **two** extension files manually as shown:

    .. image:: images/vsix.png

.. _prepsdcard:


.. .. figure:: images/store.png
..    :alt: store
..    :align: center
.. 
..    Installing the extension from the Visual Studio Code marketplace


Preparing the micro SD Card
-----------------------------------------------------------

To run MicroPython, the EV3 Intelligent Brick has to run an enhanced operating system called Ev3dev. You can run it by inserting a specially prepared micro SD card in the brick before turning it on. Removing the card and rebooting the brick will return it to it's original state.

If the micro SD card contains files you want to keep, be sure to back them up before staring this process. The process erases everything on the micro SD card, including any previous MicroPython programs on it. See :ref:`managing files on the EV3 <managefiles>` to learn how to back up your MicroPython programs if necessary.

To create the micro SD card:
1. Download the `EV3 Python firmware <.>`_ (approximately 360 MB). Do **not** open or unzip the file.
2. Download and install a micro SD card flashing tool like `Etcher <https://www.balena.io/etcher/>`_.
3. Insert the micro SD card into your computer or card reader.
4. Launch the flashing tool and follow the steps on your screen to install the firmware file you have just downloaded. If you use Etcher, you can follow the instructions in the diagram below:

    a. Select the firmware file you have just downloaded.
    b. Select your micro SD card. Make sure that the device and size correspond to your micro SD card.
    c. Start the flashing process. This may take several minutes. Do not remove the card until the flashing process is complete.

.. figure:: images/etcher.png
   :width: 85 %
   :alt: etcher
   :align: center

   Using Etcher to flash the firmware to the micro SD card

Updating from older version of EV3 MicroPython
____________________________________________________________
To update the firmware to the latest version, simply download a new image and flash it to the micro SD card as described above. Be sure to :ref:`back up any Python programs you want to save <managefiles>`.

Running the ev3dev image on the brick
-----------------------------------------------------------

Make sure the EV3 brick is turned off. Insert the micro SD card you prepared into the SD card slot on the EV3 Intelligent Brick. 

.. figure:: images/sd.png
   :width: 75 %
   :alt: sd
   :align: center

   Inserting the micro SD card into the EV3 brick


Turning the EV3 brick on and off
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Turn the EV3 on by pressing the dark gray center button. The boot process may take several minutes and will show a lot of text on the display.

Once the brick has booted, you can turn it off again. To do so, use the back button (the button just below the screen) to go to the main menu. Once at the main menu, press the back button again to show the shutdown menu. Select *Power Off* with the center button to turn the brick off. To exit the menu, press the back button again.

.. figure:: images/onoff.png
   :width: 65 %
   :alt: devicebrowser
   :align: center

   Turning the EV3 brick off


Going back to the original firmware
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can go back to the LEGO® firmware and your LEGO® programs at any time. To do so:

1. Turn the EV3 off as shown above.
2. Wait for the screen and light to turn off.
3. Remove the micro SD card.
4. Turn the EV3 on.

Viewing motor and sensor values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When you're not running a program, you can view motor and sensor values using the device browser, as shown below.

.. figure:: images/devicebrowser.png
   :width: 80 %
   :alt: devicebrowser
   :align: center

   Viewing motor and sensor values
