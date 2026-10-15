# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

from typing import Generator, Sequence, TypeVar

T = TypeVar("T")


def chunk(data: Sequence[T], size: int) -> Generator[Sequence[T], None, None]:
    """
    Divides a sequence into equal-sized chunks.

    The last chunk may be smaller than *size*.

    Args:
        data: The data to be divided.
        size: The size of each chunk.

    Returns:
        A generator that yields each chunk.
    """
    for i in range(0, len(data), size):
        yield data[i : i + size]
