try:
    import sys
except ImportError:
    sys = None

from pybricks.tools import run_task
from rfcomm_test import leader

# Get peer address from args if available
peer_addr = None
if sys and len(sys.argv) > 1:
    peer_addr = sys.argv[1]

run_task(leader(peer_addr))
