# Common modules
include("../_common/manifest.py")

# Technic Hub IMU is the same as the one in Prime Hub
freeze_as_mpy("../primehub/modules", "_imu_calibrate.py")
