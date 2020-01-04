import uos

from pybricks.parameters import Color
from pybricks.resources import Font, Image

# Working directory is top-level tests directory
TEST_IMAGE = '../ports/pybricks/tests/ev3dev/resources/test.png'

# requires one argument
try:
    Image()
except TypeError as ex:
    print(ex)

# if argument is string, load from file
img = Image(TEST_IMAGE)

# error if file does not exist
try:
    img = Image('bad.png')
except OSError as ex:
    print(ex)

# omitting file extension is OK
img = Image(TEST_IMAGE[:-4])

# if argument is Image, a copy is created
img = Image(img)

# if argument is Image and sub=True, a sub-image is created
sub = Image(img, sub=True, x1=10, y1=10, x2=20, y2=20)
print(sub.width, sub.height)

# Test properties

print(img.width)
print(img.height)


# Test clear()

img.clear()


# Test draw_pixel()

# two required arguments
img.draw_pixel(0, 0)
img.draw_pixel(x=0, y=0)
try:
    img.draw_pixel(0)
except TypeError:
    pass

# 3rd argument is kwarg
img.draw_pixel(0, 0, Color.BLACK)
img.draw_pixel(0, 0, color=Color.BLACK)


# Test draw_line()

# four required arguments
img.draw_line(0, 0, 0, 0)
img.draw_line(x1=0, y1=0, x2=0, y2=0)
try:
    img.draw_line(0, 0, 0)
except TypeError:
    pass

# 5rd argument is kwarg
img.draw_line(0, 0, 0, 0, Color.BLACK)
img.draw_line(0, 0, 0, 0, color=Color.BLACK)


# Test draw_box()

# four required arguments
img.draw_box(0, 0, 0, 0)
img.draw_box(x1=0, y1=0, x2=0, y2=0)
try:
    img.draw_box(0, 0, 0)
except TypeError:
    pass

# 5th argument is kwarg
img.draw_box(0, 0, 0, 0, 0)
img.draw_box(0, 0, 0, 0, r=0)

# 6th argument is kwarg
img.draw_box(0, 0, 0, 0, 0, False)
img.draw_box(0, 0, 0, 0, fill=False)

# 6th argument is kwarg
img.draw_box(0, 0, 0, 0, 0, False, Color.BLACK)
img.draw_box(0, 0, 0, 0, color=Color.BLACK)


# Test draw_circle()

# three required arguments
img.draw_circle(0, 0, 0)
img.draw_circle(x=0, y=0, r=0)
try:
    img.draw_circle(0, 0)
except TypeError:
    pass

# 4th argument is kwarg
img.draw_circle(0, 0, 0, False)
img.draw_circle(0, 0, 0, fill=False)

# 5th argument is kwarg
img.draw_circle(0, 0, 0, False, Color.BLACK)
img.draw_circle(0, 0, 0, color=Color.BLACK)


# Test draw_image()

# three required arguments
img.draw_image(0, 0, img)
img.draw_image(x=0, y=0, image=img)
try:
    img.draw_image(0, 0)
except TypeError:
    pass

# 4th argument is kwarg and can be Color or None
img.draw_image(0, 0, img, None)
img.draw_image(0, 0, img, Color.WHITE)
img.draw_image(0, 0, img, color=None)
img.draw_image(0, 0, img, color=Color.WHITE)


# Test draw_text()

# three required arguments
img.draw_text(0, 0, '')
img.draw_text(x=0, y=0, text='')
try:
    img.draw_text(0, 0)
except TypeError:
    pass

# 4th argument is kwarg
img.draw_text(0, 0, '', Color.BLACK)
img.draw_text(0, 0, '', color=Color.BLACK)


# Test set_font()

# one required argument
img.set_font(Font.DEFAULT)
try:
    img.set_font()
except TypeError:
    pass


# Test print()

# no required arguments
img.print()

# positional args take any object type
img.print('', 0, False, img, {}, [])

# keyword-only arg end
img.print(end='\n')
img.print('', end='\n')

# keyword-only arg sep
img.print(sep=' ')
img.print('', sep=' ')


# Test save()

# one required argument
try:
    img.save()
except TypeError as ex:
    print(ex)

# actually creates file on disk
img.save('test.png')
uos.stat('test.png')
uos.remove('test.png')

# automatically adds file extension if missing
img.save('test')
uos.stat('test.png')
uos.remove('test.png')

# upper-case is OK too
img.save('TEST.PNG')
uos.stat('TEST.PNG')
uos.remove('TEST.PNG')

# illegal name or permissions issue gives OSError
try:
    # this should fail because we are not root
    img.save('/test.png')
except OSError as ex:
    print(ex)
