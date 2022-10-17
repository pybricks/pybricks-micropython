
Setting up a development environment
------------------------------------

This is the recommended development environment used by the Pybricks maintainers.

### Requirements

- Ubuntu 22.04 (Jammy Jellyfish)
- [Git SCM][git] any recent-ish version
- [VS Code][vscode] latest version
- [Python][python] v3.10.x
- [Poetry][poetry] v1.x
- [Uncrustify][uncrustify] v0.71.x
- [GNU ARM Embedded Toolchain][arm-gcc] v10-2020-q4
- [GNU GCC][gcc] for host operating system
- [GNU Make][make]

Optional:
- [Docker][docker] (only needed if building for ev3dev)
- [Emscripten][emsdk] v1.39.x (only needed if building mpy-cross JavaScript package)
- [Yarn][yarn] v1.x (only needed if building JavaScript packages)

[git]: https://git-scm.com/downloads
[vscode]: https://code.visualstudio.com/Download
[python]: https://www.python.org/downloads/
[poetry]: https://python-poetry.org/docs/
[uncrustify]: http://uncrustify.sourceforge.net/
[arm-gcc]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
[gcc]: https://gcc.gnu.org/
[make]: https://www.gnu.org/software/make/
[docker]: https://docs.docker.com/engine/install/ubuntu/
[emsdk]: https://emscripten.org/docs/getting_started/downloads.html
[yarn]: https://classic.yarnpkg.com/en/docs/install


### Installation tips

#### Pipx

We recommend using [pipx][pipx] to install and manage python command line tools
like Poetry.

[pipx]: https://pipxproject.github.io/pipx/

Linux (Ubuntu):

    sudo apt update && sudo apt install pipx

macOS:

    brew install pipx

Windows:

    py -3 -m pip install pipx


Then install tools like this:

    pipx install poetry


#### Ubuntu

Some of the prerequisites are available as Debian packages. The correct version
of `uncrustify` is available via the Pybricks PPA. The `build-essential` package
gets the GCC compiler for the host system and Make.

    sudo apt-add-repository ppa:pybricks/ppa
    sudo apt update
    sudo apt install git python3 uncrustify build-essential

Other tools (VS Code, Poetry, ARM Embedded Toolchain, Docker) can be installed
via the recommended install methods found in the links above.

#### Windows

We use [Cygwin][cygwin] for building code on Windows. Download and install
Cygwin from the link. Select the following packages during installation:

    doxygen
    graphviz
    make
    mingw64-x86_64-gcc-core
    python3
    zip

Cygwin and the [GNU ARM Embedded Toolchain][arm-gcc] need to be added to the
`PATH` environment variable. We recommend doing this in the local terminal
rather than globally via *System Properties*. For example using PowerShell:

```powershell
$env:PATH="C:\cygwin64\bin;C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2020-q4-major\bin;$env:PATH"
```

[cygwin]: https://www.cygwin.com/

#### macOS

    brew install uncrustify
    brew instal libusb
    brew install --cask gcc-arm-embedded

### Get the code

#### Via VS Code

There is a `Git: clone` command available via the command pallette.

TODO:
- Does GitHub extension need to be installed first?
- Screenshots

#### Via GitHub Desktop

If you have never used Git before, [GitHub Desktop][ghd] is a nice tool that
does just the basics.

TODO:
- Screenshots

[ghd]: https://desktop.github.com/

#### Via command line

Note: although `pybricks-micropython` contains submodules, we don't recommend
using the `--recursive` option. There are many submodules from MicroPython that
are not used by Pybricks and take a long time to clone. Submodules will be
checkout out on-demand when running `make`.

    git clone https://github.com/pybricks/pybricks-micropython
    cd pybricks-micropython
    code . # Opens the project in VS Code

### Set up the Python environment

There are a few special Python packages needed by the build system and other
tools, so we are using Poetry to manage these requirements and set up a
virtual environment to isolate the installation so you don't break your main
Python runtime.

Open a terminal in the `pybricks-micropython` directory where you cloned the
source code, then run:

    poetry env info

Verify that the Python version listed is 3.10.x. If it is not, run:

    poetry env use <path-to-python3.10>

where `<path-to-python3.10>` is the full path to the Python 3.10 installation.

Then run the following to set up the Python environment:

    poetry install

To activate the environment, run:

    poetry shell

The command prompt will now start with `(.venv)` to remind you that you are
working in the virtual environment. You should run `poetry shell` any time you
open a new terminal window while working on `pybricks-micropython`.


Building the code
-----------------

After setting up a development environment as described above, open a terminal
in the `pybricks-micropython` directory.

If you are building firmware for a Powered Up hub, first you will need to make
sure that the cross-compiler can be found. This can be done by adding it to the
`PATH` environment variable as described in the ARM Embedded Toolchain
`readme.txt` file:

    export PATH=$PATH:$install_dir/gcc-arm-none-eabi-*/bin

Or by setting the `CROSS_COMPILE` environment variable:

    export CROSS_COMPILE=$install_dir/gcc-arm-none-eabi-*/bin/arm-none-eabi-

`$install_dir` needs to be replaced with the actual directory where you
actually installed the toolchain.

Then run:

    make <target>

Where `<target>` is one of the bricks listed in the `bricks/` directory, e.g.
`cityhub`.

Hopefully all goes well and the firmware builds. The results of the build can
be found in `bricks/<target>/build/`.


Submitting changes
------------------

We welcome [pull requests][pr] to fix bugs in `pybricks-micropython`. However,
due to the extremely limited flash memory in the Powered Up hubs, we will
probably not accept new features unless they are considered "essential". Please
open an issue to discuss first if you are not sure if something will be
acceptable.

[pr]: https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests

Changes should also follow the guidelines below:

### Coding style

For the most part, we follow the [MicroPython coding style][style]. There are
some 3rd party libraries that don't follow this style, so in those cases, the
style should match the rest of the code in the file that is being changed.

[style]: https://github.com/micropython/micropython/blob/master/CODECONVENTIONS.md

Before committing your changes, be sure to run:

    poetry run ./tools/codeformat.py

This will automatically fix common problems in files that follow the MicroPython
style.


### Logical commits

Complex changes should be split into smaller changes, each performing a logical
step towards the end goal. Each commit on its own should be able to be compiled without error. This helps code reviewers and makes finding regressions easier.


### Commit messages

Please follow the recommendations below to write a *useful* commit message.

1. Add a prefix to the subject line that gives the area of code that is changed.
   Usually this will be the relative file path of the file being changed.
    - If the change is to the pbio library, omit the `lib/` part of the path.
    - For changes to the implementation of the `pybricks` package, use import path
   instead of the file path, as in [this example](https://github.com/pybricks/pybricks-micropython/commit/f45333e76d3c946ea1a5932745fe76573263b9d6).

2. The rest of the subject line describes *what* changed.

3. The body describes *why* the change was made.

4. Include other relevant information in the commit message if applicable.
   For example, if the commit fixes a compiler error, include a copy of the
   error message.

5. Include a link to the relevant GitHub issue, if applicable.

6. Include the change in firmware size, if applicable.

7. Look at the [git history][commits] for more examples.


[commits]: https://github.com/pybricks/pybricks-micropython/commits/master


Building JavaScript packages
----------------------------

This is not needed in the course of normal development, but we have several
JavaScript packages that are build from this repository.


### firmware

This package is for distributing the Pybricks firmware.


In the `pybricks-micropython` directory:

    cd npm/firmware
    yarn install
    yarn build


### mpy-cross

This package is for distributing `mpy-cross` as a [Web Assembly][wasm] binary.
To build it, we need [Emscripten][emsdk] v1.39.12 installed. Then make sure to
activate the environment:

    source $emsdk_path/emsdk_env.sh

...where `$emsdk_path` is the folder where the Emscripten SDK is installed.

Then in the `pybricks-micropython` directory:

    # ensure that mpy-cross has been built - we need the generated files
    make -C micropython/mpy-cross

    cd npm/mpy-cross
    yarn install
    yarn build:debug

[wasm]: https://webassembly.org/

Build and deploy firmware
-------------------------

### Build the firmware

Pick your Hub from the `bricks` sub-directory you want to compile.

    poetry shell
    make mpy-cross -j8
    make -C bricks/primehub -j8

### Deploy the firmware to a hub

1. Follow the guide to prepare your Hub for Pybricks firmware installation:
   1. https://pybricks.com/install, or
   2. https://dfu.pybricks.com for Spike Prime or Mindstorms Inventor
2. Execute the deployment:
    ```shell
    make -C bricks/primehub -j8 deploy
    ```
