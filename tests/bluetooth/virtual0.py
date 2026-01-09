from pybricks.tools import run_task
from pybricks.messaging import rfcomm_scan


async def main():
    results = await rfcomm_scan(timeout=10000, num_results=10, verbose=True)
    print(results)


run_task(main())
