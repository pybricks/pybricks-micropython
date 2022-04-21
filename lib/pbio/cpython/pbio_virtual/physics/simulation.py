# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

from numpy import array, zeros, concatenate
from abc import ABC, abstractmethod


class SimulationModel(ABC):
    """Base class for describing physical systems for simulation.

    This class provides generic simulation code. Each subclass must provide
    its own equations of motion by overriding the state change function.
    """

    # These tuples should be used to name all the relevant signals.
    STATES = ("",)
    INPUTS = ("",)
    OUTPUTS = ("",)

    # Simulation time step (expressed in seconds).
    DT = 0.0001

    def __init__(self, t0=0, x0=None):
        """Initializes the system with initial conditions.

        Arguments:
            t0 (float): Start time.
            x0 (array): Initial system state.
        """

        # Save system dimensions.
        self.n = len(self.STATES)
        self.m = len(self.INPUTS)
        self.p = len(self.OUTPUTS)

        # Empty buffers to hold results, starting with the initial time/state.
        self.times = array([t0])
        self.states = zeros((self.n, 1)) if x0 is None else x0.reshape(self.n, 1)
        self.outputs = self.output(t0, x0).reshape(self.p, 1)

        # Empty buffers to hold externally set input data.
        self.input_values = zeros((self.m, 1))
        self.input_times = array([t0])

    def actuate(self, t, u):
        """Sets the actuation state of the system. This will be used by
        the simulation until something else is set.

        Arguments:
            t (float): Current time.
            u (array): Control signal vector.
        """
        self.input_times = concatenate((self.input_times, array([t])))
        self.input_values = concatenate((self.input_values, u.reshape(self.m, 1)), axis=1)

    def simulate(self, time_end):
        """Simulates the system until time_end, subject to the ongoing input.

        The system starts from the initial state. When this is called again, it
        continues from where it left off.

        Arguments:
            te (float): End time.

        Returns:
            tuple: Time (array: 1 x samples) and
                   state vector at all time samples (n x samples).
        """

        # Time samples for this segment
        t0 = self.times[-1]
        nsamples = int((time_end - t0) / self.DT) + 1
        times = array([t0 + i * self.DT for i in range(nsamples)])

        # Continue using last input.
        u = self.input_values[:, -1]

        # Create empty state vector samples
        states = zeros((self.n, nsamples))
        outputs = zeros((self.p, nsamples))

        # Start at current state
        state = self.states[:, -1]

        # Evaluate RK4 integration for all time steps
        for i, t in enumerate(times):

            # Save the state and output
            states[:, i] = state
            outputs[:, i] = self.output(t, state)

            # Single RK4 step to evaluate the next state.
            k1 = self.state_change(t, state, u)
            k2 = self.state_change(t + self.DT / 2, state + self.DT * k1 / 2, u)
            k3 = self.state_change(t + self.DT / 2, state + self.DT * k2 / 2, u)
            k4 = self.state_change(t + self.DT, state + self.DT * k3, u)
            state = state + self.DT / 6 * (k1 + 2 * k2 + 2 * k3 + k4)

        # Concatenate results
        self.times = concatenate((self.times, times))
        self.states = concatenate((self.states, states), axis=1)
        self.outputs = concatenate((self.outputs, outputs), axis=1)

    @abstractmethod
    def state_change(self, t, x, u):
        """Evaluates the equations of motion at the current state.

        This method is intended to be overridden with system-specic equations.

        Arguments:
            t (float): Current time.
            x (array): Current state vector.
            u (array): Current control signal.

        Returns:
            array: Time derivative of the state vector.
        """
        return zeros(self.n)

    @abstractmethod
    def output(self, t, x):
        """Maps the internal state to the (sensor) output values.

        Arguments:
            t (float): Current time.
            x (array): Current state vector.

        Returns:
            array: Output at the current time.
        """
        return zeros(self.p)

    def get_output_at_time(self, time, tolerance):
        """Gets the system output at a particular time.

        Arguments:
            t (float): Time at which output is requested.
            tolerance (float): How far the requested time may be ahead of
                the latest simulation time before triggering a simulation
                update.

        Returns:
            array: The output vector.
        """
        # If time is in the past, interpolate from available results.
        if time < self.times[-1]:

            # Find first index larger than given time, which always exists.
            i = next(i for i, t in enumerate(self.times) if t > time)

            # Interpolate output in time
            ratio = (time - self.times[i - 1]) / (self.times[i] - self.times[i - 1])
            return self.outputs[:, i - 1] + ratio * (self.outputs[:, i] - self.outputs[:, i - 1])

        # If time is farther into the future than our tolerance, simulate up
        # to the given time. Otherwise we avoid simulating negligible time.
        if time > self.times[-1] + tolerance:
            self.simulate(time)

        # Return latest available data.
        return self.outputs[:, -1]
