from pybricks.tools import wait, AppData

app = AppData("<bb")

app.write_bytes(b"BOO")

print("\nHello")
wait(1000)
print("World\n")
