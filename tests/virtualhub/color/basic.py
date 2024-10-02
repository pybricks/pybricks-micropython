from pybricks.parameters import Color

# "enums" should be able to be used as hash values
map = {
    Color.RED: "red",
    Color.GREEN: "green",
    Color.BLUE: "blue",
}
print(map[Color.RED])
print(map[Color.GREEN])
print(map[Color.BLUE])

# Iteration and subscription
print(*Color.BLUE)
h, s, v = Color.BLUE
print(h, s, v)
h = Color.BLUE[0]
s = Color.BLUE[1]
v = Color.BLUE[2]
print(h, s, v)

# Shifting.
print(Color.RED >> 120 == Color.GREEN)
print(Color.RED >> 720 == Color.RED)

# Scaling
light_red = Color(0, 100, 50)
print(Color.RED * 0.5 == light_red)
