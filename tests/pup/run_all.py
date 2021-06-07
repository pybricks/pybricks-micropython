#!/usr/bin/env python3

import os
import itertools
import asyncio


from pybricksdev.connections import PybricksHub
from pybricksdev.ble import find_device


async def main():

    # Get all script paths.
    scripts = []
    for (dirpath, dirnames, filenames) in os.walk("."):
        if dirpath != ".":
            scripts += [os.path.join(dirpath, name) for name in filenames if ".py" == name[-3:]]

    # Sort tests to re-run previously failed tests first.
    test_passed = lambda name: (not os.path.exists(name[:-3] + ".out"), name)
    scripts.sort(key=test_passed)

    # Establish connection.
    device = await find_device("Pybricks Hub")
    hub = PybricksHub()
    await hub.connect(device)

    # Run all scripts.
    for file_name in scripts:

        # Run it.
        print("Now running:", file_name)
        await hub.run(file_name)

        # Output file paths.
        exp_path = file_name[:-3] + ".exp"
        out_path = file_name[:-3] + ".out"

        # Remove any previous output file if it exists.
        try:
            os.remove(out_path)
        except FileNotFoundError:
            pass

        # If an expected output file exists, check that it matches.
        valid_output = True
        try:
            with open(exp_path) as exp_file:
                for exp, out in itertools.zip_longest(
                    exp_file.readlines(), hub.output, fillvalue=""
                ):
                    if exp.strip() != out.decode():
                        valid_output = False
                        break
        except FileNotFoundError:
            # If there is no expected ouput, make sure is none.
            if len(hub.output) > 0:
                valid_output = False

        # Save output if it was invalid:
        if not valid_output:
            print("Saving bad output to", out_path)
            with open(out_path, "w") as out_file:
                out_file.writelines(l.decode() + "\r\n" for l in hub.output)

            # Stop all tests. May need to make this a configurable option.
            break

    # Disconnect.
    await hub.disconnect()


asyncio.run(main())
