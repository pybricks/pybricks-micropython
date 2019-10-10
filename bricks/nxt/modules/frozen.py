mtr = Motor(Port.A)

for i in range(5):
    mtr.run_target(500, 360)
    wait(500)
    mtr.run_target(500, 0)
    wait(500)
