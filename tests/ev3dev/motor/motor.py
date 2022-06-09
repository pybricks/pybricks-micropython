from pybricks.ev3devices import Motor
from pybricks.parameters import Port

IIO_BASE = (
    "/sys/devices/platform/soc@1c00000/ti-pruss/1c32000.pru1"
    "/remoteproc/remoteproc0/virtio0/virtio0.ev3-tacho-rpmsg.-1.0"
    "/iio:device1/"
)
TACHO_BASE = (
    "/sys/devices/platform/ev3-ports/ev3-ports:outA/lego-port"
    "/port4/ev3-ports:outA:lego-ev3-l-motor/tacho-motor/motor0/"
)


def write_iio(attr, value):
    with open(IIO_BASE + attr, "w") as f:
        f.write(value + "\n")


def print_iio(attr):
    with open(IIO_BASE + attr, "r") as f:
        print(f.read().strip())


def write_tacho(attr, value):
    with open(TACHO_BASE + attr, "w") as f:
        f.write(value + "\n")


def print_tacho(attr):
    with open(TACHO_BASE + attr, "r") as f:
        print(f.read().strip())


m = Motor(Port.A)


# testing angle

print(m.angle())  # expect 0

write_iio("in_count0_raw", "360")
print(m.angle())  # expect 180

# testing stalled

print(m.control.stalled())  # expect False


# testing direct control of duty cycle

m.dc(50)
print_tacho("command")  # expect "run-direct"
print_tacho("duty_cycle_sp")  # expect 50

m.dc(100)
print_tacho("command")  # expect "run-direct"
print_tacho("duty_cycle_sp")  # expect 100

# duty cycle > 100 is clipped
m.dc(101)
print_tacho("command")  # expect "run-direct"
print_tacho("duty_cycle_sp")  # expect 100

m.dc(-100)
print_tacho("command")  # expect "run-direct"
print_tacho("duty_cycle_sp")  # expect -100

# duty cycle less than -100 is clipped
m.dc(-101)
print_tacho("command")  # expect "run-direct"
print_tacho("duty_cycle_sp")  # expect -100


# testing __str__/__repr__

print(m)
