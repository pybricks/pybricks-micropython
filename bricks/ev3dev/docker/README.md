Using Docker to Cross-Compile Pybricks MicroPython for ev3dev
-------------------------------------------------------------

This assumes that you have already `docker` installed and that you have cloned
our [MicroPython fork](https://github.com/pybricks/micropython) and its submodules.

1. Create the docker image and a docker container.

        ./docker/setup.sh armel

2. Cross compile MicroPython.

        docker exec --tty pybricks-ev3dev_armel make

3. Transfer the generated `pybricks-micropython` to the EV3 brick.

        scp bricks/ev3dev/pybricks-micropython robot@ev3dev:~

4. Run it on the EV3.

        ssh -t robot@ev3dev "brickrun -r -- ./pybricks-micropython"
