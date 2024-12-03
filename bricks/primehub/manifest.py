# Common modules
include("../_common/manifest.py")

freeze_as_mpy("../primehub/modules", "_imu_calibrate.py")
freeze_as_mpy("../primehub/modules", "_light_matrix.py")
