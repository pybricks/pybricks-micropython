Installation
===================

What do you need?
-----------------------------------------------------------

The typical configuration for developing a MicroPython program for your EV3 robot is shown in the figure below. The steps on this page show you how you can make this work.


.. figure:: images/overview.png
   :scale: 75 %
   :alt: overview
   :align: center

   Overview of the programming workflow

To get started, you'll need the following items:

- A Windows 10 or Mac OS computer.
- A micro SD card. You'll need a card with a minimum size of 4GB and a maximum size of 32GB, also known as microSDHC cards. We recommend cards with Application Performance Class A1.
- A micro SD card slot or card reader.
- A mini-USB cable. The EV3 kit includes such a cable.

You'll need internet access to complete the steps on this page. When programming your robots afterwards, internet access is not required.

Preparing your computer
-----------------------------------------------------------

You'll write your MicroPython programs using Visual Studio Code. Follow the steps below to download, install, and configure this program:

1. Download and install Visual Studio Code from `here <https://code.visualstudio.com/Download>`_.
2. Launch Visual Studio Code.
3. Install and activate the required extension as shown below:


.. figure:: images/store.png
   :alt: store
   :align: center

   Installing the extension from the Visual Studio Code marketplace

.. warning::

    The all-in-one extension is not yet available in the Visual Studio Code marketplace.
    
    Please install the **two** extension files manually as shown:

    .. image:: images/vsix.png

.. _prepsdcard:

Preparing the micro SD Card
-----------------------------------------------------------

You will now follow a series of steps to download and install the EV3 Python firmware onto your micro SD card. This "firmware" consists of `ev3dev <https://www.balena.io/etcher/>`_, along with all the tools to run Pybricks MicroPython programs.

1. Download the `EV3 Python firmware <.>`_ (approximately 360 MB) and save it in a convenient location. Do **not** open or unzip the file.
2. Download and install an micro SD card flashing tool such as `Etcher <https://www.balena.io/etcher/>`_.
3. Insert the micro SD card into your computer or card reader.
4. Launch the flashing tool and follow the steps on your screen to install the firmware file you have just downloaded. If you use Etcher, you can follow the instructions in the diagram below:

    a. Select the firmware file you have just downloaded.
    b. Select your micro SD card. Make sure that the device and size correspond to your micro SD card.
    c. Start the flashing process. This may take several minutes. Do not remove the card until the installation process is complete.

.. figure:: images/etcher.png
   :scale: 75 %
   :alt: etcher
   :align: center

   Using Etcher to flash the firmware to the micro SD card

Using the EV3 brick
-----------------------------------------------------------

Make sure the EV3 brick is turned off, and insert the micro SD card you just prepared into the SD card slot on the EV3, as shown below.

.. figure:: images/sd.png
   :scale: 60 %
   :alt: sd
   :align: center

   Inserting the finished micro SD card into the EV3 brick


Turning the EV3 brick on and off
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To turn the EV3 on, press the dark gray center button to turn the EV3 brick on. The boot process may take several minutes.

To turn it off, use the back button (the button just below the screen) to go to the main menu. Once at the main menu, press the back button to show the shutdown menu. Select *Power Off* to turn the brick off, or press the back button again to cancel.

.. figure:: images/onoff.png
   :scale: 80 %
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
   :scale: 80 %
   :alt: devicebrowser
   :align: center

   Viewing motor and sensor values
