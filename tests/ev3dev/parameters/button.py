from pybricks.parameters import Button

# "enums" print exactly the same as what we write in code "Button.RIGHT"
print(Button.RIGHT)

print(Button.LEFT == Button.LEFT)
print(Button.DOWN != Button.DOWN)
print(Button.LEFT == Button.DOWN)
print(Button.LEFT != Button.DOWN)

# Internally, these may have the same value, but externally, they are not equal
print(Button.BEACON == Button.UP)
