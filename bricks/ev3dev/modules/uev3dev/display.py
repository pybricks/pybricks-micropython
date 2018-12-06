# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Display screen"""

from fcntl import ioctl
from mmap import mmap

from framebuf import FrameBuffer
from framebuf import MONO_HLSB, RGB565, XRGB8888
from uctypes import addressof
from uctypes import sizeof
from uctypes import struct
from uctypes import ARRAY
from uctypes import UINT8
from uctypes import UINT16
from uctypes import UINT32
from uctypes import UINT64

from uev3dev._wand import CompositeOp
from uev3dev._wand import MagickWand
from uev3dev._wand import PixelWand

_FBIOGET_VSCREENINFO = 0x4600
_FBIOGET_FSCREENINFO = 0x4602

_FB_VISUAL_MONO01 = 0
_FB_VISUAL_MONO10 = 1
_FB_VISUAL_TRUECOLOR = 2

_fb_fix_screeninfo = {
    'id_name': (ARRAY | 0, UINT8 | 16),
    'smem_start': UINT32 | 16,  # long
    'smem_len': UINT32 | 20,
    'type': UINT32 | 24,
    'type_aux': UINT32 | 28,
    'visual': UINT32 | 32,
    'xpanstep': UINT16 | 36,
    'ypanstep': UINT16 | 38,
    'ywrapstep': UINT16 | 40,
    'line_length': UINT32 | 44,
    'mmio_start': UINT32 | 48,  # long
    'mmio_len': UINT32 | 52,
    'accel': UINT32 | 56,
    'capabilities': UINT16 | 60,
    'reserved0': UINT16 | 62,
    'reserved1': UINT16 | 64,
}

_fb_bitfield = {
    'offset': UINT32 | 0,
    'length': UINT32 | 4,
    'msb_right': UINT32 | 8,
}

_fb_var_screeninfo = {
    'xres': UINT32 | 0,
    'yres': UINT32 | 4,
    'xres_virtual': UINT32 | 8,
    'yres_virtual': UINT32 | 12,
    'xoffset': UINT32 | 16,
    'yoffset': UINT32 | 20,
    'bits_per_pixel': UINT32 | 24,
    'grayscale': UINT32 | 28,
    'red': (32, _fb_bitfield),
    'green': (44, _fb_bitfield),
    'blue': (56, _fb_bitfield),
    'transp': (68, _fb_bitfield),
    'nonstd': UINT32 | 80,
    'activate': UINT32 | 84,
    'height': UINT32 | 88,
    'width': UINT32 | 92,
    'accel_flags': UINT32 | 96,
    'pixclock': UINT32 | 100,
    'left_margin': UINT32 | 104,
    'right_margin': UINT32 | 108,
    'upper_margin': UINT32 | 112,
    'lower_margin': UINT32 | 116,
    'hsync_len': UINT32 | 120,
    'vsync_len': UINT32 | 124,
    'sync': UINT32 | 128,
    'vmode': UINT32 | 132,
    'rotate': UINT32 | 136,
    'colorspace': UINT32 | 140,
    'reserved0': UINT32 | 144,
    'reserved1': UINT32 | 148,
    'reserved2': UINT32 | 152,
    'reserved3': UINT32 | 156,
}


class _Screen():
    """Object that represents a screen"""

    BLACK = 0
    WHITE = ~0

    def __init__(self):
        self._fbdev = open('/dev/fb0', 'w+')
        self._fix_info_data = bytearray(sizeof(_fb_fix_screeninfo))
        fd = self._fbdev.fileno()
        ioctl(fd, _FBIOGET_FSCREENINFO, self._fix_info_data, mut=True)
        self._fix_info = struct(addressof(self._fix_info_data),
                                _fb_fix_screeninfo)
        self._var_info_data = bytearray(sizeof(_fb_var_screeninfo))
        ioctl(fd, _FBIOGET_VSCREENINFO, self._var_info_data, mut=True)
        self._var_info = struct(addressof(self._var_info_data),
                                _fb_var_screeninfo)
        self._fb_data = {}
        self._mmap = mmap(fd, self._fix_info.smem_len)

    @property
    def width(self):
        """Gets the width of the screen in pixels"""
        return self._var_info.xres_virtual

    @property
    def height(self):
        """Gets the height of the screen in pixels"""
        return self._var_info.yres_virtual

    @property
    def bpp(self):
        """Gets the color depth of the screen in bits per pixel"""
        return self._var_info.bits_per_pixel

    def update(self, data):
        """Updates the screen with the framebuffer data.

        Must be data returned by self.framebuffer().
        """
        self._mmap.seek(0)
        self._mmap.write(data)

    def framebuffer(self):
        """Creates a new framebuffer for the screen

        returns a framebuf.FrameBuffer object used for drawing and a bytearray
        object to be passed to self.update()
        """
        data = bytearray(self._fix_info.line_length * self.height)
        if self._fix_info.visual != _FB_VISUAL_TRUECOLOR:
            raise RuntimeError("Unsupported fbdev color format")
        if self.bpp == 32:
            format = XRGB8888
        elif self.bpp == 16:
            format = RGB565
        else:
            raise RuntimeError("Unsupported pixel depth")
        fbuf = FrameBuffer(data, self.width, self.height, format,
                           self._fix_info.line_length * 8 // self.bpp)
        return fbuf, data


class Display():
    """Object that represents a display screen."""
    def __init__(self):
        self._screen = _Screen()
        self._fb, self._data = self._screen.framebuffer()

    def text_pixels(self, text, clear, x, y, color, font):
        """Draw text on the screen

        Parameters:
            text (str): The text to draw
            clear (bool): When ``True``, clear the screen first
            x (int): Where to start the left side of the text
            y (int): Where to start the top of the text
            color (bool): ``True`` for white, otherwise black
            font (int): The size of the font [0..2]
        """
        if clear:
            self._fb.fill(_Screen.WHITE)
        if color:
            color = _Screen.WHITE
        else:
            color = _Screen.BLACK
        # TODO: micropython framebuf only has one font
        self._fb.text(str(text), x, y, color)
        self._screen.update(self._data)

    def image(self, image_file, clear, x, y):
        """Draw an image on the screen.

        Parameters:
            image_file (ImageFile): the image
            clear (bool): When ``True`` clear the screen first
            x (int): Where to start the left side of the image
            y (int): Where to start the top of the image

        """
        if clear:
            self._fb.fill(_Screen.WHITE)
        self._fb.blit(image_file._framebuf, x, y, 1)
        self._screen.update(self._data)

    def reset_screen(self):
        """Clear the screen"""
        self._fb.fill(_Screen.WHITE)
        self._screen.update(self._data)


class ImageFile():
    """Object that represents an image file

    Parameters:
        filename (str): The file path
    """
    def __init__(self, filename, display):
        with MagickWand() as wand:
            wand.read_image(filename)

            # make the image fit
            img_w, img_h = wand.image_width, wand.image_height
            disp_w, disp_h = display._screen.width, display._screen.height
            if img_w < disp_w and img_h < disp_h:
                # if the image is smaller than the screen, extend it with a
                # white border
                # TODO: if the screen is >= 2x the image, double the image size
                # instead of adding a border
                x = -(disp_w - img_w) // 2
                y = -(disp_h - img_h) // 2
                wand.extent_image(disp_w, disp_h, x, y)
            elif img_w > disp_w and img_h > disp_h:
                # FIXME: figure out which resize method is best and shrink the
                # image
                pass

            wand.image_format = 'GRAY'
            wand.image_depth = 1

            # then convert to micropython framebuf so we can blit
            data = wand.image_blob
            self._framebuf = FrameBuffer(data, wand.image_width,
                                         wand.image_height, MONO_HLSB)
