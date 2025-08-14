from pybricks.iodevices import I2CDevice
from pybricks.tools import wait, run_task


class UltrasonicSensor:
    def __init__(self, port):
        self._i2c = I2CDevice(port, address=0x01, powered=True, nxt_quirk=True)
        wait(100)

        try:
            assert self._i2c.read(0x08, 4) == b"LEGO"
            assert self._i2c.read(0x10, 5) == b"Sonar"
        except:
            raise OSError("Ultrasonic Sensor not found.")

    async def _distance_async(self):
        return (await self._i2c.read(0x42, 1))[0] * 10

    def distance(self):
        if run_task():
            return self._distance_async()
        return self._i2c.read(0x42, 1)[0] * 10
