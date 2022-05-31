
# Virtual hub

Pybricks has a `virtualhub` implementation that allows implementing virtual
hardware drivers in Python. This could be used for things like automated testing
and creating virtual robots in a 3-D environment.

## Building

Run:

    make virtualhub

Or if you want to pass additional options:

    make -C bricks/virtualhub ...

## Running the virtual hub

Building with the commands above will result in a `virtualhub-micropython`
executable in the `bricks/virtualhub/build/` directory. Running this executable
by itself will likely result in the error:

    ModuleNotFoundError: No module named 'pbio_virtual'

This is because it requires a module named `pbio_virtual` to be in the Python
path. Technically, it is trying to import `pbio_virtual.platform.default` but
the error only shows the first module in the chain that fails. To solve this,
set the `PYTHONPATH` environment variable to a directory containing the
`pbio_virtual` module. You can also use a different platform module by setting
the `PBIO_VIRTUAL_PLATFORM_MODULE` environment variable.

For example, this will run a virtual hub implemented using Python turtle graphics:

    PYTHONPATH=lib/pbio/cpython PBIO_VIRTUAL_PLATFORM_MODULE=pbio_virtual.platform.turtle ./bricks/virtualhub/build/virtualhub-micropython


## Internals

The `virtualhub-micropython` executable is a MicroPython runtime (based on UNIX
port). However, it also hosts a CPython runtime in the same process. The CPython
runtime only runs during callback from the MicroPython runtime, so the main
thread does not run continuously.

There are two basic kinds of hooks for passing information between the Pybricks
MicroPython runtime and the virtual driver Python runtime. For events, there are
`on_<event>()` methods in Python that will be called whenever the MicroPython
runtime emits the event. For polled values, there are properties/attributes
that are read by the MicroPython runtime whenever it needs to use the value.

## Implementing virtual hub drivers in Python

To start a new virtual hub implementation, create a Python module with a class
named `Platform`. The class needs to contain all of the required methods and
properties used by the virtual drivers enabled in the MicroPython build. See
`lib/pbio/cpython/pbio_virtual/platform/` for example implementations.
