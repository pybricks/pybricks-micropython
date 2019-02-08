#!/usr/bin/env bash

for filenamedotext in *.svg; do
    filename=$(echo "$filenamedotext" | cut -f 1 -d '.')
    inkscape --file=$filename.svg --export-area-drawing --export-png=build/$filename.png
done
