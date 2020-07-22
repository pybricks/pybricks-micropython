#!/bin/bash
#
# Creates tag in micropython submodule before rebasing so that we don't break
# things.

set -e

SCRIPT_DIR=$(dirname "$0")

cd "$SCRIPT_DIR/../micropython"

TAG=$(git describe)

git tag $TAG
git push origin $TAG
