#!/bin/bash

BUILDDIR="@CMAKE_BINARY_DIR@"
TIMEOUT="${TIMEOUT:-@EXTSORT_TEST_INT_TIMEOUT@}"
MEMLIMIT=${MEMLIMIT:-40000} # in KB
TIME_OUTPUT=${TIME_OUTPUT:-time.out}
cd "$BUILDDIR" || exit 1

echo "Configured timeout = $TIMEOUT"
echo "Configured memlimit = $MEMLIMIT"

# ref impl runs in about 2.1 s on offline test server
# total mem limit: 20000 KB = 19.5 MB, can support up to about 65-way ext-sort
# in the ref impl (i.e., the space overhead should be <= (65 - 8) * 2 * 4KB =
# 456 KB, considering the record id (8 bytes) overhead).  To sort in one pass:
# at least 1954 pages: 1000000 * 8 \approx 1954 * 4096, hence internal sort
# will definitely exceed the memory limit.
bash -c "ulimit -S -d $MEMLIMIT && /usr/bin/time -o $TIME_OUTPUT -f 'wall_time: %e s' timeout $TIMEOUT $BUILDDIR/tests/extsort/BasicTestExternalSort --gtest_filter='BasicTestExternalSort.TestSortUniformInt64Asc' --test_nitems=1000000 --test_nways 8 --buffer_pool_size=6"
EXIT_CODE=$?
cat $TIME_OUTPUT
exit $EXIT_CODE

