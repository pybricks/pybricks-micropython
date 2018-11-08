# Temporary code for the clock until we get the mechanism class done


# relative_torque_limit {int}   -- Percentage (-100.0 to 100.0) of the maximum stationary torque that the motor is allowed to produce.
# stall_speed_limit {int}       -- If this speed cannnot be reached even with the maximum torque, the motor is considered to be stalled
# stall_time {int}              -- Minimum stall time before the run_stalled action completes
# min_speed {int}               -- If speed is equal or less than this, consider the motor to be standing still
# max_speed {int}               -- Soft limit on the reference speed in all run commands
# tolerance {int}               -- Allowed deviation (deg) from target before motion is considered complete
# acceleration_start {int}      -- Acceleration when beginning to move. Positive value in degrees per second per second
# acceleration_end {int}        -- Deceleration when stopping. Positive value in degrees per second per second
# tight_loop_time {int}         -- When a run function is called twice in this interval (seconds), assume that the user is doing their own speed control.
# pid_kp {int}                  -- Proportional angle control constant (and integral speed control constant)
# pid_ki {int}                  -- Integral angle control constant
# pid_kd {int}                  -- Derivative angle control constant (and proportional speed control constant)


# All lines with TMP comment can be removed in future once we finish certain implementation aspects

second = MovehubMotor(Port.A, Dir.normal)
minute = MovehubMotor(Port.B, Dir.normal, [8, 36, 20])

sensor = ColorDistanceSensor(Port.D)

# Color, wait for red second hand
sensor.reflection() # TMP
wait(500) # TMP
second.run(200)
while(sensor.reflection() < 10):
    wait(10)
second.stop()
second.reset_angle(0)

# Ambient, wait for minute hand
sensor.ambient() #TMP
wait(500) #TMP
minute.run(-300)
while(sensor.ambient() > 20):
    wait(10)
minute.stop(Stop.hold) #TMP
wait(100) #TMP
minute.stop() # TMP
minute.reset_angle(0)
minute.run_target(500, -25)

kp = 1600
second.settings(100, 2, 500, 5, 1000, 1, 1000, 1000, 100, kp, 800, 5)
minute.settings(100, 2, 500, 5, 1000, 1, 1000, 1000, 100, kp, 800, 5)

watch = StopWatch()

while True:
    seconds = watch.time()//1000
    minutes = seconds//60
    second.run_target(1000, seconds*6)
    minute.run_target(1000, minutes*6)
