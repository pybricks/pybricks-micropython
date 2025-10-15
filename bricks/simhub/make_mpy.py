from pybricksdev.compile import compile_multi_file
import asyncio
import argparse
import sys

# Set up argument parser
parser = argparse.ArgumentParser(description="Compile to multi-mpy using pybricksdev.")
parser.add_argument("file", help="The path to the Python file to compile.")
args = parser.parse_args()

# Compile the file
result = asyncio.run(compile_multi_file(args.file, (6, 1)))

# Write the result to stdout
sys.stdout.buffer.write(result)
