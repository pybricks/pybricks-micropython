Creating and running programs
=============================

Now that you've set up your computer and EV3 brick, you're ready to start writing programs.

To make it easier to create and manage your programs, let's first have a quick look at how MicroPython projects and programs for your EV3 robots are organized.

Programs are organized into *project folders*, as shown below. A project folder is a directory on your computer that contains the main program (**main.py**) and other optional scripts or files. This project folder and all of its contents will be copied to the EV3 brick, where the main program will be run.

This page shows you how to create such a project and how to transfer it to the EV3 brick.


.. figure:: images/projectstructure.png
   :width: 100 %
   :alt: alt
   :align: center

   A project contains a program called **main.py** and optional resources like sounds or MicroPython modules.


Creating a new project
-----------------------------------------------------------

To create a new project, open the Pybricks tab and click *create a new project*, as shown below. Enter a project name in the text field that appears and press *Enter*. When prompted, choose a location for this program and confirm by clicking *choose folder*.

.. figure:: images/newproject.png
   :width: 100 %
   :alt: alt
   :align: center

   Creating a new project. This example is called *getting_started*, but you can choose any name.

When you create a new project, it already includes a file called *main.py*. To see its contents and to modify it, open it from the file browser as shown below. This is where you'll write your programs.

If you are new to MicroPython programming, we recommend that you keep the existing code in place and add your code to it.

.. figure:: images/projectoverview.png
   :width: 100 %
   :alt: alt
   :align: center

   Opening the default *main.py* program.



Connecting to the EV3 brick with Visual Studio Code
-----------------------------------------------------------

To be able to transfer your code to the EV3 brick, you'll first need to connect the EV3 brick to your computer with the mini-USB cable and configure the connection with Visual Studio Code. To do so:

- Turn the EV3 brick on
- Connect the EV3 brick to your computer with the mini-USB cable
- Configure the USB connection as shown:

.. figure:: images/connecting.png
   :width: 100 %
   :alt: alt
   :align: center

   Configuring the USB connection between the computer and the EV3 brick

Downloading and running a program
-----------------------------------------------------------

You can press the F5 key to run the program. Alternatively, you can start it manually by going to the *debug* tab and clicking the green start arrow, as shown below.

When the program starts, a pop-up toolbar allows you to stop the program if necessary. You can also stop the program at any time using the back button on the EV3 brick.

If your program produces any output with the :mod:`print <.tools>` command, this is shown in the output window.

.. figure:: images/running.png
   :width: 100 %
   :alt: alt
   :align: center

   Running a program

Expanding the example program
-----------------------------------------------------------

Now that you've run the basic code template, you can expand the program to make a motor move. First, attach a Large Motor to Port B on the EV3 brick, as shown below.


.. figure:: images/firstprogram.png
   :width: 100 %
   :alt: alt
   :align: center

   The EV3 brick with a Large Motor attached to port B.

Next, edit *main.py* to make it look like this:

.. literalinclude:: ../../examples/ev3/getting_started/main.py

This program makes your robot beep, rotate the motor, and beep again with a higher pitched tone. Run the program to make sure that it works as expected.

.. _managefiles:

Managing files on the EV3 brick
-----------------------------------------------------------

After you've downloaded a project to the EV3 brick, you can run, delete, or back up programs stored on it using the device browser:

.. figure:: images/files.png
   :width: 100 %
   :alt: alt
   :align: center

   Using the device browser to manage files on your EV3 brick


Running a program without a computer
-----------------------------------------------------------

You can run previously downloaded programs directly from the EV3 brick.

To do so, find the program using the *file browser* on the EV3 screen and press the center button key to start the program.

.. figure:: images/manualrun.png
   :width: 100 %
   :alt: alt
   :align: center

   Starting a program using the buttons on the EV3 brick
