#!/bin/sh
# deploy python package to self-hosted PyPi

set -e

cat << EOF > $HOME/.pypirc
[distutils]
index-servers = local

[local]
repository: https://pybricks.jfrog.io/pybricks/api/pypi/pybricks-pypi
username: travis-ci
password: $JFROG_SECRET
EOF

cd $TRAVIS_BUILD_DIR/lib/fake-pybricks
python3 setup.py sdist upload -r local
