#!/bin/sh

set -e

script_dir=$(dirname $(readlink -f $0))

# sh args: hub_name, fw_file
# py args: hub_name, timestamp, build_id, commit_id, size
$script_dir/upload-fw-size.py "$1" "$(date --utc '+%F %T')" "$GITHUB_RUN_ID" \
    "$(git rev-parse HEAD)" "$(wc -c $2 | cut -f 1 -d ' ')"
