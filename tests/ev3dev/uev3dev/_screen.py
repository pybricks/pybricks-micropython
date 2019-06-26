from pybricks.uev3dev.display import _Screen

s = _Screen()

print(s.width)  # 178
print(s.height)  # 128
print(s.bpp)  # 32

print(s._fix_info.line_length)  # 178 * 4 = 712
print(s._fix_info.visual)  # _FB_VISUAL_TRUECOLOR = 2
