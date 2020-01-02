
from pybricks.hubs import EV3Brick
from pybricks.parameters import Color

ev3 = EV3Brick()

# Test contants

print(ev3.screen.WIDTH)
print(ev3.screen.HEIGHT)


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
