Adapted from [docker for lms2012](https://github.com/ev3dev/lms2012-compat/blob/ev3dev-stretch/docker/README.md).


Using Docker to Cross-Compile the MicroPython unix port
--------------------------------------------

This assumes that you have already `docker` installed and that you have cloned
our [MicroPython fork](https://github.com/laurensvalk/micropython) and its submodules. 
The current working directory is the `micropython/ports/pybricks/bricks/EV3`
source code directory.

1. Create the docker image and a docker container.

        ./docker/setup.sh armel

2.  Get an interactive shell with the working directory in bricks/EV3

        docker run --rm -it -v /abs/path/to/micropython:/micropython -w /micropython/ports/pybricks/bricks/EV3 pybricks-ev3-armel

3. Cross compile micropython

        make CROSS_COMPILE=arm-linux-gnueabi-

4. Exit when ready

        exit

5. Transfer the generated MicroPython application (from the EV3 folder) to the EV3 brick and run it.