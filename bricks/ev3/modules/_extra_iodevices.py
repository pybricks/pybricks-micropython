from pybricks.iodevices import BluetoothClassicGamepad
from pybricks.parameters import Button

# Report ID of the basic report that both controllers send over Bluetooth.
REPORT_ID = 0x01

# (byte index, bit index, Button) for the two main button bytes.
BUTTON_MAP = (
    (5, 4, Button.X),  # Square
    (5, 5, Button.A),  # Cross
    (5, 6, Button.B),  # Circle
    (5, 7, Button.Y),  # Triangle
    (6, 0, Button.LB),  # L1
    (6, 1, Button.RB),  # R1
    (6, 4, Button.VIEW),  # Share / Create
    (6, 5, Button.MENU),  # Options
    (6, 6, Button.LJ),  # L3
    (6, 7, Button.RJ),  # R3
    (7, 0, Button.GUIDE),  # PS
    (7, 1, Button.UPLOAD),  # Touchpad click
)

# Dpad directions per hat value 0-7. Value 8 means nothing pressed.
DPAD_MAP = (
    (Button.UP,),
    (Button.UP, Button.RIGHT),
    (Button.RIGHT,),
    (Button.RIGHT, Button.DOWN),
    (Button.DOWN,),
    (Button.DOWN, Button.LEFT),
    (Button.LEFT,),
    (Button.LEFT, Button.UP),
)


class Keypad:
    """Mimics the buttons attribute of the other Pybricks controller classes."""

    def __init__(self, pressed):
        self.pressed = pressed


class PlayStationController(BluetoothClassicGamepad):
    def __init__(self, joystick_deadzone=10):
        super().__init__()
        self.joystick_deadzone = joystick_deadzone
        self.buttons = Keypad(self.pressed)

    def _report(self):
        report = self.report()
        if len(report) < 10 or report[0] != REPORT_ID:
            raise OSError("Unexpected report. Is this a PlayStation controller?")
        return report

    def pressed(self):
        report = self._report()
        buttons = set()

        for index, bit, button in BUTTON_MAP:
            if report[index] & (1 << bit):
                buttons.add(button)

        hat = report[5] & 0x0F
        if hat < 8:
            buttons.update(DPAD_MAP[hat])

        return buttons

    def dpad(self):
        """Returns 0 for nothing pressed, then 1-8 clockwise starting at up."""
        hat = self._report()[5] & 0x0F
        return 0 if hat > 7 else hat + 1

    def _joystick(self, x_raw, y_raw):
        # Axes are 8-bit unsigned with 128 at the center.
        x = (x_raw - 128) * 100 // 127
        y = (128 - y_raw) * 100 // 127

        # Square deadzone to prevent drift.
        if abs(x) < self.joystick_deadzone and abs(y) < self.joystick_deadzone:
            return (0, 0)

        return (x, y)

    def joystick_left(self):
        report = self._report()
        return self._joystick(report[1], report[2])

    def joystick_right(self):
        report = self._report()
        return self._joystick(report[3], report[4])

    def triggers(self):
        report = self._report()
        return (report[8] * 100 // 255, report[9] * 100 // 255)
