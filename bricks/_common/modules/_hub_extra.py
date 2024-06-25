from pybricks.parameters import Axis
from pybricks.tools import wait


def light_matrix_text_async(display, text, on, off):
    for char in text:
        display.char(char)
        yield from wait(on)
        display.off()
        yield from wait(off)


def imu_update_heading_correction(hub):

    # Number of turns to confirm the result.
    NUMBER_CONFIRM_TURNS = 5

    # Maximum speed values before we consider the result invalid.
    MAX_XY_SPEED = 50
    MAX_Z_SPEED = 800

    # Routine to wait on a button, with some extra time to avoid vibration directly after.
    def wait_for_click():
        while hub.buttons.pressed():
            wait(1)
        while not hub.buttons.pressed():
            wait(1)
        print("Processing...")
        while hub.buttons.pressed():
            wait(1)
        wait(1500)

    # Disable stop buttons
    hub.system.set_stop_button(None)
    print(
        "Put the hub on a flat table. Align against a fixed reference like a wall or heavy book. Press hub button when ready."
    )

    # Wait for fixed reference and store the initial angle value.
    wait_for_click()
    while not hub.imu.ready() or not hub.imu.stationary():
        wait(1)
    start_z = hub.imu.rotation(-Axis.Z)

    # Wait for a full rotation and get the result.
    print(
        "Keep the hub flat and slide it to make a full turn clockwise. Put it against the same reference. Press hub button when ready."
    )
    wait_for_click()
    one_turn = hub.imu.rotation(-Axis.Z) - start_z

    # Require clockwise...
    if one_turn < 0:
        raise ValueError("You turned it the wrong way. Please try again.")

    # Sanity check. Should be close to 360.
    if not (350 < one_turn < 370):
        print(one_turn)
        raise ValueError(
            "The error was more than 10 degrees, which is unexpected. Please try again."
        )

    # Instruct to make more turns.
    print("So far so good! Now make", NUMBER_CONFIRM_TURNS, "full clockwise turns.")

    for i in range(NUMBER_CONFIRM_TURNS):

        # The rotation target is just under a rotation so we can show the next
        # message to keep going in time, avoiding doubts about what to do.
        target = one_turn * (i + 2) - 10

        # Wait until the hub reaches the target.
        while hub.imu.rotation(-Axis.Z) - start_z < target:
            wait(1)

            if hub.buttons.pressed():
                raise RuntimeError("Don't press the button until all turns are complete!")

            if abs(hub.imu.angular_velocity(Axis.Z)) > MAX_Z_SPEED:
                raise RuntimeError("Not so fast! Try again.")

            if (
                abs(hub.imu.angular_velocity(Axis.X)) + abs(hub.imu.angular_velocity(Axis.Y))
                > MAX_XY_SPEED
            ):

                raise RuntimeError("Please keep the hub flat! Try again.")

        # Inform user of status.
        print("Completed", i + 1, "out of", NUMBER_CONFIRM_TURNS, "turns. ", end="")
        if i < NUMBER_CONFIRM_TURNS - 1:
            print("Keep going!")
        else:
            print("Put it against the same reference. Press hub button when ready.")

    # Wait for final confirmation.
    wait_for_click()

    # Verify the result.
    expected = one_turn * (NUMBER_CONFIRM_TURNS + 1)
    result = hub.imu.rotation(-Axis.Z) - start_z

    if abs(expected - result) > NUMBER_CONFIRM_TURNS / 2:
        print(
            "The",
            NUMBER_CONFIRM_TURNS,
            "extra turns where different from the first turn. Try again.",
        )
        print("Expected", expected, "but got", result)

    # Get the final result to save.
    average_turn = result / (NUMBER_CONFIRM_TURNS + 1)
    print("For every 360-degree turn your gyro Z-axis reports:", average_turn)
    hub.imu.settings(heading_correction=average_turn)
    print("Saved! Now the heading() method will report the adjusted value.")
