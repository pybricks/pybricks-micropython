#!/bin/bash
#
# Creates tag in stm32lib submodule before rebasing so that we don't break
# things.

set -e

SCRIPT_DIR=$(dirname "$0")

cd "$SCRIPT_DIR/../micropython/lib/stm32lib"

# Describe against the upstream vendor tags only, otherwise a previous
# pybricks- tag is found and the prefix is added a second time.
TAG=pybricks-$(git describe --tags --exclude 'pybricks-*')

git tag $TAG
git push origin $TAG
