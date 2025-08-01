#!/usr/bin/env python3

import math
import numpy
import sympy
import scipy.linalg
import textwrap

# Scaled motor angle, angular velocity, angular acceleration, and current
th_bar = sympy.Symbol("theta_bar")
w_bar = sympy.Symbol("omega_bar")
i_bar = sympy.Symbol("i_bar")

# Scaled input voltage and torque
V_bar = sympy.Symbol("V_bar")
tau_bar = sympy.Symbol("tau_bar")

# Unit scalers
c_th = 180 * 1000 / sympy.pi  # millidegrees
c_tau = 1e6  # microNewtonmeters
c_V = 1e3  # millivolts
c_i = 1e4  # tenths of milliAmperes

# Actual Motor angle, angular velocity, angular acceleration, and current (SI)
th = th_bar / c_th
w = w_bar / c_th
i = i_bar / c_i

# Actual input voltage and torque (SI)
V = V_bar / c_V
tau = tau_bar / c_tau

# Motor inertia, resistance, inductance, backemf constant, torque constant (SI)
In = sympy.Symbol("In", positive=True)
R = sympy.Symbol("R", positive=True)
L = sympy.Symbol("L", positive=True)
Ke = sympy.Symbol("Ke", positive=True)
Kt = sympy.Symbol("Kt", positive=True)

# Equations of motion (SI)
dth_dt = w
di_dt = V / L - Ke / L * w - R / L * i
dw_dt = Kt / In * i - tau / In

# Steady state input/output conversions for legacy observer conversions.
dtau_dv = Kt / R * c_tau / c_V
dv_dtau = 1 / dtau_dv
dtau_dw = Kt / R * Ke * c_tau / c_th
dtau_da = In * c_tau / c_th

# System state
x = sympy.Matrix(
    [
        [th_bar],
        [w_bar],
        [i_bar],
    ]
)

# System input
u = sympy.Matrix(
    [
        [V_bar],
        [tau_bar],
    ]
)

# State change f(x)
dx = sympy.Matrix([[c_th * dth_dt], [c_th * dw_dt], [c_i * di_dt]])

Phi = dx.jacobian(x)
Gam = dx.jacobian(u)

exponent = sympy.Matrix(
    [sympy.Matrix([[Phi, Gam]]), sympy.zeros(Gam.shape[1], Gam.shape[1] + Phi.shape[0])]
)

# Upper limits for known LEGO devices with rotation sensors.
MAX_SI_SPEED = 2500 / 180 * sympy.pi
MAX_SI_ACCELERATION = MAX_SI_SPEED / 0.1
MAX_SI_CURRENT = 3.0
MAX_SI_VOLTAGE = 12
MAX_SI_TORQUE = 1

# Maximum numeric value that we might reach.
MAX_NUM_SPEED = math.floor((MAX_SI_SPEED * c_th).evalf())
MAX_NUM_ACCELERATION = math.floor((MAX_SI_ACCELERATION * c_th).evalf())
MAX_NUM_CURRENT = math.floor((MAX_SI_CURRENT * c_i))
MAX_NUM_VOLTAGE = math.floor((MAX_SI_VOLTAGE * c_V))
MAX_NUM_TORQUE = math.floor((MAX_SI_TORQUE * c_tau))

# Prescalers such that signal * prescale does not exceed INT32_MAX.
INT32_MAX = 2147483647
PRESCALE_SPEED = INT32_MAX // MAX_NUM_SPEED
PRESCALE_ACCELERATION = INT32_MAX // MAX_NUM_ACCELERATION
PRESCALE_CURRENT = INT32_MAX // MAX_NUM_CURRENT
PRESCALE_VOLTAGE = INT32_MAX // MAX_NUM_VOLTAGE
PRESCALE_TORQUE = INT32_MAX // MAX_NUM_TORQUE

HEADER = textwrap.dedent(
    f"""
    // Values generated by pbio/doc/control/model.py
    #define MAX_NUM_SPEED ({MAX_NUM_SPEED})
    #define MAX_NUM_ACCELERATION ({MAX_NUM_ACCELERATION})
    #define MAX_NUM_CURRENT ({MAX_NUM_CURRENT})
    #define MAX_NUM_VOLTAGE ({MAX_NUM_VOLTAGE})
    #define MAX_NUM_TORQUE ({MAX_NUM_TORQUE})
    #define PRESCALE_SPEED ({PRESCALE_SPEED})
    #define PRESCALE_ACCELERATION ({PRESCALE_ACCELERATION})
    #define PRESCALE_CURRENT ({PRESCALE_CURRENT})
    #define PRESCALE_VOLTAGE ({PRESCALE_VOLTAGE})
    #define PRESCALE_TORQUE ({PRESCALE_TORQUE})

    typedef struct _pbio_observer_model_t {{
        int32_t d_angle_d_speed;
        int32_t d_speed_d_speed;
        int32_t d_current_d_speed;
        int32_t d_angle_d_current;
        int32_t d_speed_d_current;
        int32_t d_current_d_current;
        int32_t d_angle_d_voltage;
        int32_t d_speed_d_voltage;
        int32_t d_current_d_voltage;
        int32_t d_angle_d_torque;
        int32_t d_speed_d_torque;
        int32_t d_current_d_torque;
        int32_t d_voltage_d_torque;
        int32_t d_torque_d_voltage;
        int32_t d_torque_d_speed;
        int32_t d_torque_d_acceleration;
        int32_t torque_friction;
        int32_t feedback_gain;
    }} pbio_observer_model_t;"""
)


def make_model(name, *, V, tau_0, tau_x, w_0, w_x, i_0, i_x, a, Lm, h):
    """Initialize the model using experimental data"""

    # Compute system parameters from motor curve data:
    model = {}
    model[Kt] = (tau_x - tau_0) / (i_x - i_0)
    tau_s = model[Kt] * i_0 - tau_0
    R_over_Ke = model[Kt] * (w_x - w_0) / (tau_0 - tau_x)

    model[Ke] = V / (w_0 + R_over_Ke / model[Kt] * (tau_0 + tau_s))
    model[R] = R_over_Ke * model[Ke]

    # Get inertia and inductance from experimental data:
    model[In] = (model[Kt] * V / model[R] - tau_s) / a
    model[L] = Lm

    # Substitute parameters into model to get numeric system matrices
    exponent_numeric = numpy.array(exponent.subs(model).evalf().tolist()).astype(
        numpy.float64
    )

    # Get matrix exponential and system matrices
    exponential = scipy.linalg.expm(exponent_numeric * h)
    A = exponential[0:3, 0:3]
    B = exponential[0:3, 3:5]

    # Matrix multiplication goes like this, e.g. for the first row:
    #
    # angle_next += a_01 * speed + a_02 * current ....
    #
    # The matrix entries a_ij are floating point values, but we want to
    # evaluate everything using 32-bit signed integers. So instead of
    # storing the floating point values, we store their inverse, prescaled
    # by an appropriate scaling factor:
    #
    # angle_next = speed_prescale * speed / (speed_prescale / a_01)
    #
    # The term (speed_prescale / a_01) is stored as a single integer.
    #
    return textwrap.dedent(
        f"""
        static const pbio_observer_model_t model_{name} = {{
            .d_angle_d_speed = {round(PRESCALE_SPEED / A[0, 1])},
            .d_speed_d_speed = {round(PRESCALE_SPEED / A[1, 1])},
            .d_current_d_speed = {round(PRESCALE_SPEED / A[2, 1])},
            .d_angle_d_current = {round(PRESCALE_CURRENT / A[0, 2])},
            .d_speed_d_current = {round(PRESCALE_CURRENT / A[1, 2])},
            .d_current_d_current = {round(PRESCALE_CURRENT / A[2, 2])},
            .d_angle_d_voltage = {round(PRESCALE_VOLTAGE / B[0, 0])},
            .d_speed_d_voltage = {round(PRESCALE_VOLTAGE / B[1, 0])},
            .d_current_d_voltage = {round(PRESCALE_VOLTAGE / B[2, 0])},
            .d_angle_d_torque = {round(PRESCALE_TORQUE / B[0, 1])},
            .d_speed_d_torque = {round(PRESCALE_TORQUE / B[1, 1])},
            .d_current_d_torque = {round(PRESCALE_TORQUE / B[2, 1])},
            .d_voltage_d_torque = {round(PRESCALE_TORQUE / dv_dtau.subs(model).evalf())},
            .d_torque_d_voltage = {round(PRESCALE_VOLTAGE / dtau_dv.subs(model).evalf())},
            .d_torque_d_speed = {round(PRESCALE_SPEED / dtau_dw.subs(model).evalf())},
            .d_torque_d_acceleration = {round(PRESCALE_ACCELERATION / dtau_da.subs(model).evalf())},
            .torque_friction = {round(tau_s * c_tau)},
        }};"""
    )


if __name__ == "__main__":
    print(HEADER)

    print(
        make_model(
            name="technic_m_angular",
            # Voltage in this experiment:
            V=7.2,
            # Torque load of 1018.64 g * cm:
            tau_x=1018.64 / 1000 * 9.81 / 100,
            i_x=0.51,
            w_x=4.6,
            # No load condition:
            tau_0=0,
            i_0=0.09,
            w_0=16.6,
            # Estimated acceleration and inductance:
            a=math.radians(920 / 0.035),
            Lm=0.0008 * 30,
            # System model sample time
            h=0.005,
        )
    )
