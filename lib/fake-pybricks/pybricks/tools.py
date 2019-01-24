"""Common tools for timing and datalogging."""


def print():
    """print(value, ..., sep, end, file, flush)

    Print values on the terminal or a stream.

    Example::

        # Print some text
        print("Hello, world")

        # Print some text and a number
        print("Value:", 5)

    """
    pass


def wait(time):
    """Pause the user program for a specified amount of time.

    Arguments:
        time (:ref:`time`): How long to wait.

    """
    pass


class StopWatch():
    """A stopwatch to measure time intervals. Similar to the stopwatch feature on your phone."""

    def __init__(self):
        pass

    def time(self):
        """Get the current time of the stopwatch.

        Returns:
            :ref:`time`: Elapsed time.
        """
        pass

    def pause(self):
        """Pause the stopwatch."""
        pass

    def resume(self):
        """Resume the stopwatch."""
        pass

    def reset(self):
        """Reset the stopwatch time to 0.

        The run state is unaffected:

        * If it was paused, it stays paused (but now at 0).
        * If it was running, it stays running (but starting again from 0).
        """
        pass
