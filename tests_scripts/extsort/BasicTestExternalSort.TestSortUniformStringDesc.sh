#!/bin/bash

BUILDDIR="@CMAKE_BINARY_DIR@"
TIMEOUT="${TIMEOUT:-@EXTSORT_TEST_STRING_TIMEOUT@}"
MEMLIMIT="${MEMLIMIT:-110000}"
TIME_OUTPUT=${TIME_OUTPUT:-time.out}
cd "$BUILDDIR" || exit 1

echo "Configured timeout = $TIMEOUT"
echo "Configured memlimit = $MEMLIMIT"

# ref impl runs in about 5.2 s on offline test server
# total mem limit: 55000 KB = 53.7 MB, can support up to about 768-way ext-sort
# in the ref impl (i.e., the space overhead should be <= (768 - 16) * 358/350 *
# 4 = 3077 KB = 3 MB, considering the record id (8 bytes) overhead).  To
# sort in one pass: at least 8545 pages: 100000 * 350 \approx 8545 * 4096
# (actually 8572 pages with default seed). Hence, internal sort will definitely
# exceed the memory limit.
bash -c "ulimit -S -d $MEMLIMIT && /usr/bin/time -o '$TIME_OUTPUT' -f 'wall_time: %e s' timeout $TIMEOUT $BUILDDIR/tests/extsort/BasicTestExternalSort --gtest_filter='BasicTestExternalSort.TestSortUniformStringDesc' --test_nitems=100000 --test_nways 16 --buffer_pool_size=6"
EXIT_CODE=$?
cat "$TIME_OUTPUT"
exit $EXIT_CODE

