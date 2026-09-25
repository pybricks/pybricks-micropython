Frozen modules that only some bricks have the underlying support for, so each
brick's manifest.py freezes them explicitly instead of the blanket glob in
../manifest.py picking them up everywhere.
