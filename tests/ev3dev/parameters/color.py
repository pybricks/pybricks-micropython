from pybricks.parameters import Color

# "enums" should be able to be used as hash values
map = {
    Color.RED: 'red',
    Color.GREEN: 'green',
    Color.BLUE: 'blue',
}
print(map[Color.RED])
print(map[Color.GREEN])
print(map[Color.BLUE])
