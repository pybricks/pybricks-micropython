## Quick links

- :arrow_right: [Try the latest **stable** release][Pybricks Code].
- :arrow_right: [Try the latest **beta** release][Pybricks Code Beta].
- :arrow_right: You can now [**sponsor**][sponsors] this project!

## Issues & discussion
- :arrow_right: [Issues for all Pybricks repositories](https://github.com/pybricks/support/issues).
- :arrow_right: [Discussions for all Pybricks repositories](https://github.com/orgs/pybricks/discussions).

# Pybricks

Pybricks brings Python-like coding to programmable LEGO bricks, and transforms
remote-controlled smart hubs into truly autonomous robots.

Pybricks builds on [MicroPython][micropython/micropython], which is an
efficient Python implementation that runs on microcontrollers. Pybricks expands
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

All modern Powered Up hubs can be used with [Pybricks Code],
our online IDE:

- LEGO Technic Hub
- LEGO BOOST Move Hub
- LEGO City Hub
- LEGO SPIKE Prime Hub
- LEGO SPIKE Essential Hub
- LEGO MINDSTORMS Robot Inventor Hub

We also support Pybricks v2.x on MINDSTORMS EV3. The community-supported version
comes preinstalled with the latest [ev3dev image]. It is also available via
the [LEGO Education website][lego-education-ev3-micropython].

# Development

Most Pybricks users will not need to do anything with this source
code. [Pybricks Code][Pybricks Code] always gives you the latest stable
firmware. You can even try the [beta release][Pybricks Code Beta] to check out the latest and greatest features.

But since you're reading this, you might be interested to build the code
yourself, or start tweaking and hacking. Check out
the [contributor's guide](./CONTRIBUTING.md) to get started.

## Pybricks and Upstream MicroPython

The Pybricks authors are actively contributing to upstream MicroPython instead
of forking it into a whole new project. In essence, Pybricks just adds a family
of `ports`.

To streamline development and releases, we do maintain a fork of `micropython`
[here][pybricks/micropython]. This adds a few minor patches that haven't made
it upstream yet. Every once in a while, that repository gets rebased with all
the latest and greatest that upstream [`micropython`][micropython/micropython]
has to offer.

Summing up:

- [`pybricks/pybricks-micropython`][pybricks-micropython]: Main repo for all
  Pybricks firmware. This has a continuous master branch.
- [`pybricks/micropython`][pybricks/micropython]: fork of
  [upstream MicroPython][micropython/micropython] that is included as a
  submodule in the main `pybricks-micropython` repository.
  This gets occasionally rebased when a new MicroPython release comes out.


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
- [`pybricks-code`][pybricks-code]: This is the source code for the
  Pybricks Code online IDE.

## What about the other stuff in /bricks?

Like MicroPython, Pybricks can run on just about every device that lets
you update the firmware. And we could not resist doing just that, so there's
more than just the bricks/hubs mentioned above. But getting to a point where it
is easy to
use for everyday users takes quite a bit more work,
so not all bricks in `/bricks` will be supported officially for now.
Be sure to join the discussion on our [issue tracker][issue tracker] to
share your thoughts on these new ports.

## Differences with other LEGO MicroPython Implementations

Read more about the differences on the Pybricks [about page].

# How can I help?
If you've got any of the hubs listed above, the best way to help right now is
to try out the beta release of [Pybricks Code][Pybricks Code Beta]. You may run
into issues or bugs. Please let us know by opening an [issue][issue tracker].
Thanks!

If you enjoy using Pybricks, please
consider [sponsoring the project][sponsors].

[pybricks-micropython]: https://github.com/pybricks/pybricks-micropython
[pybricks/micropython]: https://github.com/pybricks/micropython

[pybricks-api]: https://github.com/pybricks/pybricks-api
[pybricks-projects]: https://github.com/pybricks/pybricks-api
[pybricks-code]: https://github.com/pybricks/pybricks-code

[micropython/micropython]: https://github.com/micropython/micropython

[pybricks.com]: https://pybricks.com
[docs]: https://docs.pybricks.com

[ev3dev image]: https://www.ev3dev.org/downloads
[ev3dev.org]: https://www.ev3dev.org/
[ev3dev-lang]: https://github.com/ev3dev/ev3dev-lang-python

[lego-education-ev3-micropython]: https://education.lego.com/en-us/support/mindstorms-ev3/python-for-ev3

[Pybricks Code]: https://code.pybricks.com
[Pybricks Code Beta]: https://beta.pybricks.com
[about page]: https://pybricks.com/about/

[alpha version]: https://github.com/pybricks/support/issues/167
[issue tracker]: https://github.com/pybricks/support/issues
[roadmap]: https://github.com/pybricks/support/issues/29
[sponsors]: https://github.com/sponsors/pybricks
