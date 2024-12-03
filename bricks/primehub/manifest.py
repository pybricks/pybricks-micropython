import pathlib

# Common modules
include("../_common/manifest.py")

# PrimeHub additions, like IMU.
primehub_modules = list(pathlib.Path("../primehub/modules").glob("*.py"))

if any(primehub_modules):
    for m in primehub_modules:
        print(f"Including {m.name} as a module.")
        freeze_as_mpy(str(m.parent), m.name)
