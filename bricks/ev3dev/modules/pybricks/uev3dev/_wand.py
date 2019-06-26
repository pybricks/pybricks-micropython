# SPDX-License-Identifier: MIT
# Copyright (c) 2017 David Lechner <david@lechnology.com>

"""Wrapper around libmagickwand
"""

import ffi
from ustruct import calcsize
from ustruct import unpack

from uctypes import bytearray_at

_wand = ffi.open('libMagickWand-6.Q16.so.3')
_genisis = _wand.func('v', 'MagickWandGenesis', '')
_terminus = _wand.func('v', 'MagickWandTerminus', '')
_new = _wand.func('p', 'NewMagickWand', '')
_destroy = _wand.func('p', 'DestroyMagickWand', 'p')
_clear_exception = _wand.func('i', 'MagickClearException', 'p')
_get_exception = _wand.func('s', 'MagickGetException', 'pp')
# _get_exception_type = _wand.func('I', 'MagickGetExceptionType', 'p')
_relinquish_memory = _wand.func('p', 'MagickRelinquishMemory', 'p')
_read_image = _wand.func('I', 'MagickReadImage', 'pP')
_write_image = _wand.func('I', 'MagickWriteImage', 'pP')
_export_image_pixels = _wand.func('i', 'MagickExportImagePixels', 'pPPPPPIp')
_reset_iterator = _wand.func('v', 'MagickResetIterator', 'p')
_border_image = _wand.func('i', 'MagickBorderImage', 'pPPPI')
_extent_image = _wand.func('i', 'MagickExtentImage', 'pPPPP')

_get_image_width = _wand.func('p', 'MagickGetImageWidth', 'p')
_get_image_height = _wand.func('p', 'MagickGetImageHeight', 'p')
_get_image_depth = _wand.func('p', 'MagickGetImageDepth', 'p')
_set_image_depth = _wand.func('i', 'MagickSetImageDepth', 'pP')
_get_image_format = _wand.func('s', 'MagickGetImageFormat', 'p')
_set_image_format = _wand.func('i', 'MagickSetImageFormat', 'pP')
_get_image_blob = _wand.func('p', 'MagickGetImageBlob', 'pp')
_get_image_gravity = _wand.func('I', 'MagickGetImageGravity', 'p')
_set_image_gravity = _wand.func('i', 'MagickSetImageGravity', 'pI')

# global library init (a well-mannered program would call _terminus() later)
_genisis()


class MagickWandError(Exception):
    def __init__(self, severity, desc):
        super(MagickWandError, self).__init__('{}: {}'.format(severity, desc))


class MagickWand():
    """Object for image manipulation using ImageMagick

    .. note:: Since micropython does not support ``__del__()`` on user-defined
        classes, either use a ``with`` statement or manually call ``__del__()``
        to free the underlying resources.

    """
    def __init__(self):
        self._wand = _new()

    # micropython doesn't support __del__ in user-defined classes, so this
    # won't run on GC/finalization, but we use the name __del__ anyway.
    def __del__(self):
        if self._wand:
            self._wand = _destroy(self._wand)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, traceback):
        self.__del__()

    def _raise_error(self):
        severity = bytearray(calcsize('I'))
        desc = _get_exception(self._wand, severity)
        # FIXME: this leaks desc (need to call _relinquish_memory()) but not
        # sure how to marshal pointer to string
        severity = unpack('I', severity)[0]
        _clear_exception(self._wand)
        raise MagickWandError(severity, desc)

    def read_image(self, filename):
        """Read an image from a file

        Parameters:
            filename (str): the file path
        """
        ok = _read_image(self._wand, str(filename))
        if not ok:
            self._raise_error()

    def write_image(self, filename):
        """Write an image to a file

        Parameters:
            filename (str) the file path
        """
        ok = _write_image(self._wand, str(filename))
        if not ok:
            self._raise_error()

    @property
    def image_width(self):
        """Gets the width of the current image in pixels"""
        return _get_image_width(self._wand)

    @property
    def image_height(self):
        """Gets the height of the current image in pixels"""
        return _get_image_height(self._wand)

    def image_depth(self):
        """Gets the depth of the current image in bits per pixel (8, 16 or 32)
        """
        return _get_image_depth(self._wand)

    def _set_image_depth(self, depth):
        ok = _set_image_depth(self._wand, depth)
        if not ok:
            self._raise_error()

    image_depth = property(image_depth, _set_image_depth)

    def image_format(self):
        """Gets the image format"""
        return _get_image_format(self._wand)

    def _set_image_format(self, format):
        ok = _set_image_format(self._wand, str(format))
        if not ok:
            self._raise_error()

    image_format = property(image_format, _set_image_format)

    def image_gravity(self):
        """Gets the image gravity"""
        return _get_image_gravity(self._wand)

    def _set_image_gravity(self, gravity):
        ok = _set_image_gravity(self._wand, int(gravity))
        if not ok:
            self._raise_error()

    image_gravity = property(image_gravity, _set_image_gravity)

    @property
    def image_blob(self):
        """Gets the image raw binary data"""
        _reset_iterator(self._wand)
        length = bytearray(calcsize('P'))
        data = _get_image_blob(self._wand, length)
        try:
            length = unpack('P', length)[0]
            # It is a bit inefficient to copy the data, but it is needed for
            # seamless memory management
            return bytearray_at(data, length)[:]
        finally:
            _relinquish_memory(data)

    def export_image_pixels(self, x, y, columns, rows, map_, storage, pixels):
        """Extracts pixel data

        Parameters:
            x (int): the left bound
            y (int): the top bound
            columns (int): the width
            rows (int): the height
            map (str): the pixel ordering, e.g. "RGB"
            storage (StorageType): the pixel data type
            pixels (bytearray): something big enough to hold all of the data
        """
        ok = _export_image_pixels(self._wand, x, y, columns, rows, map_,
                                  storage, pixels)
        if not ok:
            self._raise_error()

    def border_image(self, border_color, width, height, compose):
        """Surrounds the image with a border"""
        ok = _border_image(self._wand, border_color._wand, int(width),
                           int(height), int(compose))
        if not ok:
            self._raise_error()

    def extent_image(self, width, height, x, y):
        """Extentds the image using the current background color

        Parameters:
            width (int): the new width
            height (int): the new height
            x (int): the horizontal offset
            y (int): the vertical offset

        """
        ok = _extent_image(self._wand, int(width), int(height), int(x), int(y))
        if not ok:
            self._raise_error()

_new_pixel_wand = _wand.func('p', 'NewPixelWand', '')
_destroy_pixel_wand = _wand.func('p', 'DestroyPixelWand', 'p')
_pixel_get_exception = _wand.func('s', 'PixelGetException', 'pp')
_pixel_clear_exception = _wand.func('i', 'PixelClearException', 'p')

_pixel_get_color = _wand.func('s', 'PixelGetColorAsString', 'p')
_pixel_set_color = _wand.func('i', 'PixelSetColor', 'pP')


class PixelError(Exception):
    def __init__(self, severity, desc):
        super(PixelError, self).__init__('{}: {}'.format(severity, desc))


class PixelWand():
    def __init__(self):
        self._wand = _new_pixel_wand()

    def __del__(self):
        self._wand = _destroy_pixel_wand(self._wand)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, traceback):
        self.__del__()

    def color(self):
        return _pixel_get_color(self._wand)

    def _set_color(self, color):
        ok = _pixel_set_color(self._wand, str(color))
        if not ok:
            self._raise_error()

    def _raise_error(self):
        severity = bytearray(calcsize('I'))
        desc = _pixel_get_exception(self._wand, severity)
        # TODO: we could be leaking desc here (the docs are not clear if it
        # needs to be freed or not)
        severity = unpack('I', severity)[0]
        _pixel_clear_exception(self._wand)
        raise PixelError(severity, desc)


class StorageType():
    UNDEFINED = 0
    CHAR = 1
    DOUBLE = 2
    FLOAT = 3
    LONG = 4
    LONG_LONG = 5
    QUANTUM = 6
    SHORT = 7


class Gravity():
    UNDEFINED = 0
    FORGET = 1
    NORTH_WEST = 2
    NORTH = 3
    NORTH_EAST = 4
    WEST = 5
    CENTER = 6
    EAST = 7
    SOUTH_WEST = 8
    SOUTH = 9
    SOUTH_EAST = 10


class CompositeOp():
    UNDEFINED = 0
    ALPHA = 1
    ATOP = 2
    BLEND = 3
    BLUR = 4
    BUMPMAP = 5
    CHANGE_MASK = 6
    CLEAR = 7
    COLOR_BURN = 8
    COLOR_DODGE = 9
    COLORIZE = 10
    COPY_BLACK = 11
    COPY_BLUE = 12
    COPY = 14
    COPY_CYAN = 15
    COPY_GREEN = 16
    COPY_MAGENTA = 17
    COPY_ALPHA = 18
    COPY_RED = 18
    COPY_YELLOW = 19
    DARKEN = 20
    DARKEN_INTENSITY = 21
    DIFFERENCE = 22
    DISPLACE = 23
    DISSOLVE = 24
    DISTORT = 25
    DIVIDE_DST = 26
    DIVIDE_SRC = 27
    DST_ATOP = 28
    DST = 29
    DST_IN = 30
    DST_OUT = 31
    DST_OVER = 32
    EXCLUSION = 33
    HARD_LIGHT = 34
    HARD_MIX = 35
    HUE = 36
    IN = 37
    INTENSITY = 38
    LIGHTEN = 39
    LIGHTEN_INTENSITY = 40
    LINEAR_BURN = 41
    LINEAR_DODGE = 42
    LINEAR_LIGHT = 43
    LUMINIZE = 44
    MATHEMATICS = 45
    MINUS_DST = 46
    MINUS_SRC = 47
    MODULATE = 48
    MODULUS_ADD = 49
    MODULUS_SUBTRACT = 50
    MULTIPLY = 51
    NO = 52
    OUT = 53
    OVER = 54
    OVERLAY = 55
    PEGTOP_LIGHT = 56
    PINLIGHT = 57
    PLUS = 58
    REPLACE = 59
    SATURATE = 60
    SCREEN = 61
    SOFT_LIGHT = 62
    SRC_ATOP = 63
    SRC = 64
    SRC_IN = 65
    SRC_OUT = 66
    SRC_OVER = 67
    THRESHOLD = 68
    VIVID_LIGHT = 69
    XOR = 70
