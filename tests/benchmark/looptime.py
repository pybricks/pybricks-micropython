# Typical ingredients for a "segway program": Reading sensors and setting duty in a fast loop. Just some random formula for now.

left = Motor(Port.A)
right = Motor(Port.B)
watch = StopWatch()
for i in range(0, 10000):
    avg_pos = left.angle() + right.angle()
    formula = i // 100 - avg_pos // 36
    left.duty(formula)
    right.duty(formula)

watch.pause()

left.coast()
right.coast()

print("usec/loop:", watch.time() // 10)  # 433 us/loop as of 817bc7b
