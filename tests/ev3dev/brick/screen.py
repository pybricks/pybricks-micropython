
from pybricks.hubs import EV3Brick
from pybricks.parameters import Color
from pybricks.media.ev3dev import Font

ev3 = EV3Brick()

# Test contants

print(ev3.screen.width)
print(ev3.screen.height)


# Test clear()

ev3.screen.clear()


# Test draw_pixel()

# two required arguments
ev3.screen.draw_pixel(0, 0)
ev3.screen.draw_pixel(x=0, y=0)
try:
    ev3.screen.draw_pixel(0)
except TypeError:
    pass

# 3rd argument is kwarg
ev3.screen.draw_pixel(0, 0, Color.BLACK)
ev3.screen.draw_pixel(0, 0, color=Color.BLACK)


# Test draw_line()

# four required arguments
ev3.screen.draw_line(0, 0, 0, 0)
ev3.screen.draw_line(x1=0, y1=0, x2=0, y2=0)
try:
    ev3.screen.draw_line(0, 0, 0)
except TypeError:
    pass

# 5rd argument is kwarg
ev3.screen.draw_line(0, 0, 0, 0, Color.BLACK)
ev3.screen.draw_line(0, 0, 0, 0, color=Color.BLACK)


# Test draw_box()

# four required arguments
ev3.screen.draw_box(0, 0, 0, 0)
ev3.screen.draw_box(x1=0, y1=0, x2=0, y2=0)
try:
    ev3.screen.draw_box(0, 0, 0)
except TypeError:
    pass

# 5th argument is kwarg
ev3.screen.draw_box(0, 0, 0, 0, 0)
ev3.screen.draw_box(0, 0, 0, 0, r=0)

# 6th argument is kwarg
ev3.screen.draw_box(0, 0, 0, 0, 0, False)
ev3.screen.draw_box(0, 0, 0, 0, fill=False)

# 6th argument is kwarg
ev3.screen.draw_box(0, 0, 0, 0, 0, False, Color.BLACK)
ev3.screen.draw_box(0, 0, 0, 0, color=Color.BLACK)


# Test draw_circle()

# three required arguments
ev3.screen.draw_circle(0, 0, 0)
ev3.screen.draw_circle(x=0, y=0, r=0)
try:
    ev3.screen.draw_circle(0, 0)
except TypeError:
    pass

# 4th argument is kwarg
ev3.screen.draw_circle(0, 0, 0, False)
ev3.screen.draw_circle(0, 0, 0, fill=False)

# 5th argument is kwarg
ev3.screen.draw_circle(0, 0, 0, False, Color.BLACK)
ev3.screen.draw_circle(0, 0, 0, color=Color.BLACK)


# Test draw_text()

# three required arguments
ev3.screen.draw_text(0, 0, '')
ev3.screen.draw_text(x=0, y=0, text='')
try:
    ev3.screen.draw_text(0, 0)
except TypeError:
    pass

# 4th argument is kwarg
ev3.screen.draw_text(0, 0, '', Color.BLACK)
ev3.screen.draw_text(0, 0, '', color=Color.BLACK)


# Test set_font()

# one required argument
ev3.screen.set_font(Font.DEFAULT)
try:
    ev3.screen.set_font()
except TypeError:
    pass


# Test print()

# no required arguments
ev3.screen.print()

# positional args take any object type
ev3.screen.print('', 0, False, ev3.screen, {}, [])

# keyword-only arg end
ev3.screen.print(end='\n')
ev3.screen.print('', end='\n')

# keyword-only arg sep
ev3.screen.print(sep=' ')
ev3.screen.print('', sep=' ')
