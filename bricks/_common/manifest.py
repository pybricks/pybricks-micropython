import pathlib

modules = list(pathlib.Path("./modules").glob("*.py"))

if any(modules):
    for m in modules:
        path, file = m.parts
        print(f"Including {m.stem} as a module.")
        freeze_as_mpy(path, file)
