# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk
# Copyright (c) 2018-2019 David Lechner
# Copyright (c) 2019 LEGO System A/S

"""Pybricks module for showing text or images on a display (e.g. EV3 screen)"""
# TODO: This module should go elsewhere since it isn't EV3 specific.

from .uev3dev.display import Display as Ev3devDisplay
from .uev3dev.display import ImageFile
from .parameters import Align


class Display():
    """Show images or text on a display."""
    _valid_devices = ['EV3']
    _font_height = 8 # TODO: Update class when font no longer constant

    def __init__(self, device_type):
        """Device specific display initialization."""
        assert device_type in self._valid_devices, 'Selected device is not supported.'
        if device_type == 'EV3':
            self._device = Ev3devDisplay()
        self._loaded_files = {}
        self._reset_text_history()
        self.clear()

    def _reset_text_history(self):
        self._prev_text_location = (0, 0)

    def _next_line(self):
        x_prev, y_prev = self._prev_text_location
        y_next = y_prev + self._font_height
        if y_next >= self._device._screen.height:
            self._device.scroll()
            return (x_prev, y_prev)
        else:
            return (x_prev, y_next)

    def clear(self):
        """Clear everything on the screen."""
        self._device.reset_screen()
        self._reset_text_history()

    def text(self, text, location=None):
        """Write text on the screen.

        Parameters:
            text {str}: The text to display.
            location {tuple}: (x, y) coordinate. If no location is specified, it just continues on the next line.

        """
        # Parse location argument
        if isinstance(location, tuple) and len(location) == 2:
            # If coordinates are specified, use these directly
            x, y = location
        else:
            # Otherwise, add text on next line
            x, y = self._next_line()

        # TODO: clear rectangle with same size as text?
        self._device.text_pixels(text, 0, x, y, 0, 2)

        self._prev_text_location = (x, y)

    def image(self, file_name, location_or_alignment=Align.CENTER, clear=True):
        """Show an image file on the display.

        Arguments:
            file_name {str} -- Path to the image file.
            location_or_alignment {(x, y) coordinate or alignment} -- Where to place the image. (default: {Align.CENTER})
            clear {bool} -- Clear the screen before showing the image. (default: {True})
        """
        # Load the file unless already loaded
        if file_name not in self._loaded_files:
            self._loaded_files[file_name] = ImageFile(file_name, self._device)

        height = self._loaded_files[file_name].height
        width = self._loaded_files[file_name].width

        # Parse location argument
        if isinstance(location_or_alignment, tuple) and len(location_or_alignment) == 2:
            # Use the specififed coordinates directly
            x, y = location_or_alignment
        else:
            # Otherwise, get x, y based on location and image size
            align = location_or_alignment
            assert Align.BOTTOM_LEFT <= align <= Align.TOP_RIGHT, "Invalid location."

            # Find horizontal location
            if align in [Align.LEFT, Align.BOTTOM_LEFT, Align.TOP_LEFT]:
                x = 0
            elif align in [Align.RIGHT, Align.BOTTOM_RIGHT, Align.TOP_RIGHT]:
                x = self._device._screen.width - width
            else:
                x = (self._device._screen.width - width) // 2

            # Find vertical location
            if align in [Align.TOP, Align.TOP_LEFT, Align.TOP_RIGHT]:
                y = 0
            elif align in [Align.BOTTOM, Align.BOTTOM_LEFT, Align.BOTTOM_RIGHT]:
                y = self._device._screen.height - height
            else:
                y = (self._device._screen.height - height) // 2

        self._device.image(self._loaded_files[file_name], clear, x, y)
