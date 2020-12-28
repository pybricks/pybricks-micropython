# Pybricks v2

Pybricks brings Python-like coding to programmable LEGO bricks, and transforms
remote-controlled smart hubs into truly autonomous robots.

Pybricks builds on [MicroPython][micropython/micropython], which is an efficient
Python implementation that runs on microcontrollers. Pybricks expands
MicroPython with new powerful drivers for LEGO devices
including motors and sensors. It also adds the `pybricks` package, which makes
it easy for end-users to access those devices and all the features of the smart
hubs.

*Are you a beginning user?*

Start by visiting our [website][pybricks.com] and
the [documentation][docs].

*Are you a developer or just super curious?*

Read on!

# Supported Programmable Bricks & Hubs

This branch is for Pybricks v2.x which only supports LEGO MINDSTORMS EV3. This
is the version of Pybricks that is available in [ev3dev-stretch][ev3dev.org]
and from [LEGO Education][lego-education-ev3-micropython].

For Powered Up hubs, see the latest version of Pybricks
[here][pybricks-micropython] instead.


## Pybricks and Upstream MicroPython

We are actively contributing to upstream MicroPython instead of forking it into
a whole new project. We are adding Pybricks as a new family of ports. Similar
to how MicroPython already has a `ports/stm32` folder with multiple
`boards`, we add a `ports/pybricks` folder with multiple programmable `bricks`.

To streamline development and releases, we do maintain a fork of `micropython`
[here][pybricks/micropython]. It essentially just adds the repository that you
are reading right now, as a git submodule. It also adds a few minor patches.
Every once in a while, that repository gets updated with all the latest and
greatest that upstream [`micropython`][micropython/micropython] has to offer.

Summing up:

- [`pybricks/micropython`][pybricks/micropython]: fork of
  [upstream MicroPython][micropython/micropython] that just adds the repo below
  as a submodule at `ports/pybricks`. It does not have a continuous master
  branch, just versioned tags. So, never do `git pull`, but do `git fetch` and
  then check out the tag you want.
- [`pybricks/pybricks-micropython`][pybricks-micropython]: Main repo for all
  Pybricks firmware. This has a continuous master branch that never breaks.
  Always look at the [tag file](micropython-tag) to see which MicroPython tag
  must be checked out to build successfully.


## Cloning

As explained above, this
repository ([`pybricks/pybricks-micropython`][pybricks-micropython])
is a submodule to [our fork of MicroPython][pybricks/micropython]. So instead
of cloning this repository directly, do this:

    git clone --recursive https://github.com/pybricks/micropython

Then you will find this repo at `ports/pybricks` in the `micropython` directory
that was just cloned. Almost all development is done in here.
The [`micropython-tag`](micropython-tag) file tells you which tag to checkout
in [`pybricks/micropython`][pybricks/micropython].

## What about all the other repositories?
Pybricks includes not just firmware for each LEGO hub, but also various tools,
documentation, and fun projects. Here's an overview of the other repositories:

- [`pybricks-api`][pybricks-api]: This is the Pybricks user API. It documents
  the `pybricks` package that comes preinstalled in our firmware. This
  repository doesn't contain any real code. The real package is written in C
  and baked into the firmware. This repo just helps us design and document the
  user Python API without getting into too much implementation details. Don't
  want to build it? View the web version [here][docs].
- [`pybricks-projects`][pybricks-projects]: This is a broad
  collection of end-user MicroPython scripts that you can run! This includes
  example snippets and projects for official LEGO models and custom made ones.

## What about the other stuff in /bricks?

Like MicroPython, Pybricks can run on just about every device that lets
you update the firmware. And we could not resist doing just that, so there's
more than just the bricks above. But getting to a point where it is easy to
use for everyday users takes quite a bit more work,
so not all bricks in `/bricks` will be supported officially for now.

# There are so many (Micro)Pythons!

We agree, and that's why we do not fork, but expand and help improve
MicroPython. Check out [the development section](#development) to
see what this means in terms of source code.

This section instead aims to clarify the differences with other
LEGO-compatible MicroPython variants.

Combined, these aspects make Pybricks truly unique:

1. Pybricks can run on **all** upgradeable bricks and smart hubs.
2. Pybricks has the **same end-user [API][docs]** across all platforms.
3. Pybricks user scripts run **autonomously** on all hubs, instead of being
  remote-controlled by an external device. This is about **100x faster**.
4. Pybricks device drivers are written in C and built into the MicroPython
  firmware, instead of being written in Python user-space. This makes them
  memory efficient and **much faster**.
5. Pybricks comes with precise motor control and drive base **synchronization
  tools**, and easy ways to use them.
6. Pybricks makes sensors and motors **cross-platform compatible**. If the
   cable fits, then it should just work.

The following sections go into some more detail of other solutions for various
platforms.

## Differences with the official LEGO Education EV3 MicroPython

This one's easy, because it's the same: The official
[LEGO Education EV3 MicroPython solution][lego-education-ev3-micropython]
uses the Pybricks library running on ev3dev.

## Differences with other ev3dev-based implementations

[ev3dev][ev3dev.org] is based on Debian Linux, which lets you access LEGO
motors and sensors by reading from and writing to system files. This has
spurred many developers to create language-specific libraries that do this for
you.

The main differences between these implementations compared to Pybricks are
points 1, 2, 4, 5, and 6 listed above. However, they let you use the
EV3 with other programming languages, notably including Python 3
using [ev3dev lang Python][ev3dev-lang]. This can be beneficial if your
project requires libraries that MicroPython does not have.

## Differences with LEGO Education SPIKE PRIME MicroPython

We think it's super exciting that LEGO is also working on MicroPython.

A Pybricks version for SPIKE Prime is under consideration as well. It would
work just like our other Pybricks firmwares. Notable differences with the
official firmware would be points 1, 2, and 6 listed above. At the same time,
this means that not all SPIKE Prime features will initially be supported.

We recommend that especially teachers and students start with the official apps
and MicroPython solutions provided by LEGO. Anyone who wants to take the next
step is welcome to try Pybricks.

[pybricks-micropython]: https://github.com/pybricks/pybricks-micropython
[pybricks/micropython]: https://github.com/pybricks/micropython

[pybricks-api]: https://github.com/pybricks/pybricks-api
[pybricks-projects]: https://github.com/pybricks/pybricks-api

[micropython/micropython]: https://github.com/micropython/micropython

[pybricks.com]: https://pybricks.com
[docs]: https://pybricks.github.io/ev3-micropython/

[ev3dev.org]: https://www.ev3dev.org/
[ev3dev-lang]: https://github.com/ev3dev/ev3dev-lang-python

[lego-education-ev3-micropython]: https://education.lego.com/en-us/support/mindstorms-ev3/python-for-ev3

