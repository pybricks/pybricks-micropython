from pybricks.tools import _set_run_loop_active


def all(*tasks):
    completed = {t: False for t in tasks}
    all_done = False
    while not all_done:
        all_done = True
        for t in tasks:
            if not completed[t]:
                all_done = False
                try:
                    next(t)
                except StopIteration:
                    completed[t] = True
                yield


def run(main_task):
    _set_run_loop_active(True)
    try:
        for _ in main_task:
            pass
    finally:
        _set_run_loop_active(False)
