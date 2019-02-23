Creating and running programs
=============================

Now that you've set up your computer and your EV3 brick, you're ready to start writing programs.

Creating a new project
-----------------------------------------------------------

To create a new project, open the Pybricks tab and click *create a new project*. Enter a project name in the text field that appears and press *Enter*. When prompted, choose a location for this program and confirm by clicking *choose folder*. 

.. figure:: images/newproject.png
   :width: 100 %
   :alt: alt
   :align: center

   Creating a new project. This example is called *getting_started*, but you can choose any name.

When you create a new project the extension automatically generates a *main.py* file. You can write your code inside the main.py file. To see its contents and to modify it, open it from the file browser as shown below.

If you are new to Python or MicroPython programming, we recommend that you keep the existing code in place only add to it.

.. figure:: images/projectoverview.png
   :width: 100 %
   :alt: alt
   :align: center

   Opening the default *main.py* program.


Program & file organisation
----------------------------------------------
Programs are organized into *project folders*. A project folder is a directory on your computer that contains the main program (**main.py**), and other optional scripts or files. This project folder and all of its contents will be copied to the EV3, where the main program will be run.

.. figure:: images/projectstructure.png
   :width: 100 %
   :alt: alt
   :align: center

   A project contains a program called **main.py** and optional resources like sounds or Python modules.


Connecting to the EV3 Intelligent Brick from within VS Code
-----------------------------------------------------------

To transfer your code to the EV3 brick and run it, you can connect it to your computer. Make sure that your EV3 brick is turned on. Then follow the steps below to configure the connection.

.. figure:: images/connecting.png
   :width: 100 %
   :alt: alt
   :align: center

   Configuring the connection between the computer and the EV3 Intelligent Brick

Downloading and running a program
-----------------------------------------------------------

You can press the F5 key to run the example program. Alternatively, you can start it manually by going to the *debug* tab, and clicking the green start arrow.

When the program starts, a pop up toolbar allows you to stop the program. If your program produces any output with the `print()` command, this is shown in the output window.

.. figure:: images/running.png
   :width: 100 %
   :alt: alt
   :align: center

   Running a program


Expanding the example program
-----------------------------------------------------------

Now that you've run the basic code template you can expand the program to make a motor move. First, attach a Large Motor to Port B on the EV3 brick, as shown below.


.. figure:: images/firstprogram.png
   :width: 100 %
   :alt: alt
   :align: center

   The EV3 brick with a Large Motor attached to port B.

Next, edit *main.py* and make it look like this:

.. literalinclude:: ../../examples/ev3/getting_started/main.py

What you've done is programmed the brick to beep, rotate the motor and beep again, this time longer and higher. Run the program to see if worked.

.. _managefiles:

Managing files on the EV3
-----------------------------------------------------------

After you've downloaded a project to the EV3, you can run, delete, or back up programs via the VS Code extension.

.. figure:: images/files.png
   :width: 100 %
   :alt: alt
   :align: center

   View of the file system on a connected EV3 brick.


Running a program without a computer
-----------------------------------------------------------

You can also start it directly from the brick, after you've downloaded it with VS Code.

Simply select *file browser* in the EV3 brick menu and use the *center* button (enter) to start the program.

.. figure:: images/manualrun.png
   :width: 100 %
   :alt: alt
   :align: center

   Starting a program using the buttons on the EV3 brick
