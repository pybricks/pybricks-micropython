# Pybricks MicroPython tests

Currently, the tests in the `ev3dev` and `virtualhub` folders are automated
while the remaining folders contain tests that must be run manually.

Use `./test-ev3dev.sh` in the top-level directory to run the automated tests
for ev3dev (requires Linux).

Use `./test-virtualhub.sh` in the top-level directory to run the automated tests
for virtualhub.

Use `--list-test` to list tests or `--include <regex>` to run single tests.

Use `--clean-failures` to remove previous failure logs.

Set environment variable `COVERAGE=1` to run code coverage (virtualhub only).
Report can be viewed at `bricks/virtualhub/build-coverage/html/index.html`.
