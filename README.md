# Pybricks

Firmware source code of Pybricks-compatible devices

This repository is a submodule of our fork of MicroPython. So instead of
cloning this repository directly, do this:

    git clone --recursive https://github.com/pybricks/micropython

Then you will find this repo at `ports/pybricks` in the `micropython` directory
that was just cloned.

By default, this will exclude the large submodules for the Pybricks EV3RT port. If you want to download those as well, go to `bricks/ev3rt` and do this:

    git submodule update --checkout --init --recursive

We may occasionally rebase the `pybricks/micropython` repository. To update
to the latest version, run the following commands:

    git checkout master
    # this assumes that origin is https://github.com/pybricks/micropython and not a fork
    git fetch origin
    git reset --hard origin/master
    git submodule update
    cd ports/pybricks
    git checkout master
    git submodule update
