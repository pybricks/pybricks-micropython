#!/bin/sh

set -e

script_dir=$(dirname $(readlink -f $0))

if [ "$TRAVIS_BRANCH" = "master" ]; then
    python3 -m pip install --quiet --user pygsheets

    # sh args: hub_name, fw_file
    # py args: hub_name, timestamp, build_id, commit_id, size
    $script_dir/upload-fw-size.py "$1" "$(date --utc '+%F %T')" "$TRAVIS_JOB_NUMBER" \
        "$(git rev-parse HEAD)" "$(wc -c $2 | cut -f 1 -d ' ')"
fi
