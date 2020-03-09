from pybricks.media.ev3dev import Font

print(Font.DEFAULT.family)
print(Font.DEFAULT.style)
print(Font.DEFAULT.width)
print(Font.DEFAULT.height)
print(Font.DEFAULT.text_width("test"))
print(Font.DEFAULT.text_height("test"))

# all args are optional
Font()

# 1st arg can be None or str
Font("sans-serif")
Font(None)
Font(family="sans-serif")
Font(family=None)

# 2nd arg is int
Font(None, 12)
Font(size=12)

# 3rd arg is bool
Font(None, 12, False)
Font(bold=False)

# 4th arg is bool
Font(None, 12, False, False)
Font(monospace=False)

# 5th arg is None or str
Font(None, 12, False, False, None)
Font(None, 12, False, False, "en-US")
Font(lang=None)
Font(lang="en-US")

# 6th arg is None or str with len() 4
Font(None, 12, False, False, None, None)
Font(None, 12, False, False, None, "Latn")
Font(script=None)
Font(script="Latn")
try:
    Font(script="bad")
except ValueError:
    pass
