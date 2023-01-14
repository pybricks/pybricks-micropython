Any .py scripts included in this folder will be included in the firmware for
all hubs.

For example, if you add a file called `my_code.py`, you can use it
with `import my_code`.

This lets you include large libraries in the firmware. Now you can use them
without having to download them each time you run a program.

If you include modules for the first time, be sure to clean the
build (e.g. `make clean-primehub`). After that, any changes to your modules
should be picked up automatically each time you build or deploy as usual.
