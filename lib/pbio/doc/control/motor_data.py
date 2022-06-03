#!/usr/bin/env python3

import math
from motor_model import HEADER, make_model

# Portion of the header that goes in <pbio/observer.h>
print(HEADER)


def rpm_to_rad_s(rpm):
    return rpm / 60 * 360 / 180 * math.pi


print("\n#if PBIO_CONFIG_SERVO_PUP")

# Observer data structures for each motor
print(
    make_model(
        # Data from experiments by Pybricks authors
        name="technic_s_angular",
        V=6,
        tau_x=318.24 / 1000 * 9.81 / 100,
        i_x=0.22,
        w_x=5.5,
        tau_0=0,
        i_0=0.05,
        w_0=13.3,
        a=math.radians(880 / 0.04),
        Lm=0.0008 * 30,
        h=0.005,
        gain=500,
    )
)

print(
    make_model(
        # Data from experiments by Pybricks authors
        name="technic_m_angular",
        V=7.2,
        tau_x=1018.64 / 1000 * 9.81 / 100,
        i_x=0.51,
        w_x=4.6,
        tau_0=0,
        i_0=0.09,
        w_0=16.6,
        a=math.radians(920 / 0.035),
        Lm=0.0008 * 30,
        h=0.005,
        gain=2000,
    )
)

print(
    make_model(
        # Data from experiments by Pybricks authors
        name="technic_l_angular",
        V=7.2,
        tau_x=1018.64 / 1000 * 9.81 / 100,
        i_x=0.53,
        w_x=12.7,
        tau_0=0,
        i_0=0.1,
        w_0=16.6,
        a=math.radians(800 / 0.04),
        Lm=0.0004 * 30,
        h=0.005,
        gain=4000,
    )
)


print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="interactive",
        V=9,
        tau_x=4.08 / 100,
        i_x=0.19,
        w_x=rpm_to_rad_s(171),
        tau_0=0,
        i_0=0.041,
        w_0=rpm_to_rad_s(255),
        a=math.radians(3000 / 0.1),
        Lm=0.0002 * 30,
        h=0.005,
        gain=2000,
    )
)

print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="technic_l",
        V=9,
        tau_x=8.81 / 100,
        i_x=0.52,
        w_x=rpm_to_rad_s(198),
        tau_0=0,
        i_0=0.120,
        w_0=rpm_to_rad_s(315),
        a=math.radians(3000 / 0.1),
        Lm=0.0003 * 30,
        h=0.005,
        gain=2500,
    )
)

print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="technic_xl",
        V=9,
        tau_x=8.81 / 100,
        i_x=0.47,
        w_x=rpm_to_rad_s(198),
        tau_0=0,
        i_0=0.06,
        w_0=rpm_to_rad_s(330),
        a=math.radians(3000 / 0.1),
        Lm=0.0002 * 30,
        h=0.005,
        gain=2000,
    )
)

print("\n#if PBIO_CONFIG_SERVO_PUP_MOVE_HUB")

print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="movehub",
        V=9,
        tau_x=4.08 / 100,
        i_x=0.37,
        w_x=rpm_to_rad_s(264),
        tau_0=0,
        i_0=0.140,
        w_0=rpm_to_rad_s(350),
        a=math.radians(3000 / 0.1),
        Lm=0.0002 * 30,
        h=0.005,
        gain=2000,
    )
)

print("\n#endif // PBIO_CONFIG_SERVO_PUP_MOVE_HUB")

print("\n#endif // PBIO_CONFIG_SERVO_PUP")

print("\n#if PBIO_CONFIG_SERVO_EV3_NXT")

print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="ev3_l",
        V=9,
        tau_x=17.3 / 100,
        i_x=0.69,
        w_x=rpm_to_rad_s(105),
        tau_0=0,
        i_0=0.06,
        w_0=rpm_to_rad_s(175),
        a=math.radians(1000 / 0.1),
        Lm=0.0005 * 30,
        h=0.005,
        gain=4000,
    )
)

print(
    make_model(
        # Partially based on https://www.philohome.com/motors/motorcomp.htm
        name="ev3_m",
        V=9,
        tau_x=6.64 / 100,
        i_x=0.37,
        w_x=rpm_to_rad_s(165),
        tau_0=0,
        i_0=0.08,
        w_0=rpm_to_rad_s(260),
        a=math.radians(2000 / 0.1),
        Lm=0.0005 * 30,
        h=0.005,
        gain=2000,
    )
)
print("\n#endif // PBIO_CONFIG_SERVO_EV3_NXT")
