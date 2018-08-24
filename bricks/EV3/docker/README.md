Adapted from [docker for lms2012](https://github.com/ev3dev/lms2012-compat/blob/ev3dev-stretch/docker/README.md)


Using Docker to Cross-Compile the MicroPython unix port
--------------------------------------------

This assumes that you have already `docker` installed and that you have cloned
our [MicroPython fork](https://github.com/laurensvalk/micropython) and its submodules. 
The current working directory is the `micropython/ports/pybricks/bricks/EV3`
source code directory.

1. Create the docker image and a docker container.

        ./docker/setup.sh armel

2.  Then build the code.

        docker exec -t lms2012_armel make
        docker exec -t lms2012_armel make install

### Tips

* To get an interactive shell to the container, run 

        docker exec -it lms2012_armel bash

* When you are done building, you can stop the container.

        docker stop --time 0 lms2012_armel

    `docker exec ...` will not work until you start the container again.

        docker start lms2012_armel

    And the container can be deleted when you don't need it anymore (don't
    forget to stop it first).

        docker rm lms2012_armel
