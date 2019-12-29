# sphinx-doc config file to build doxygen docs

import subprocess
subprocess.run(['doxygen', 'doxygen.conf'], check=True)

html_extra_path = ['build/html']
