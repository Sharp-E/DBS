#!/bin/bash

BUILDDIR="@CMAKE_BINARY_DIR@"
TIMEOUT="${TIMEOUT:-1000}" # defaults to 1000 s
MEMLIMIT=${MEMLIMIT:-65536} # in KB, defaults to 64 MB
TIME_OUTPUT=${TIME_OUTPUT:-time.out}
cd "$BUILDDIR" || exit 1

echo "Configured timeout = $TIMEOUT"
echo "Configured memlimit = $MEMLIMIT"

echo "ulimit -S -d $MEMLIMIT && /usr/bin/time -o $TIME_OUTPUT -f 'wall_time: %e s' timeout $TIMEOUT $@" | bash
EXIT_CODE=$?
cat $TIME_OUTPUT
exit $EXIT_CODE

