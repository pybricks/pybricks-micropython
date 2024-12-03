import pathlib

modules = list(pathlib.Path("./modules").glob("*.py"))

for m in modules:
    freeze_as_mpy(str(m.parent), m.name)
