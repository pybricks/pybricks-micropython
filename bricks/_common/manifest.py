import pathlib

modules = list(pathlib.Path("./modules").glob("*.py"))

if any(modules):
    for m in modules:
        print(f"Including {m.name} as a module.")
        freeze_as_mpy(str(m.parent), m.name)
