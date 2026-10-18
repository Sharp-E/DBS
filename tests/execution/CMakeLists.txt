# tests/query/CMakeLists.txt

add_tdb_test(BasicTestTableScan)
add_tdb_test(BasicTestTempTable)
add_tdb_test(BasicTestProjection)
add_tdb_test(BasicTestLimit)

add_tdb_test(BasicTestSelection)
add_tdb_test(BasicTestAggregation)
add_tdb_test(BasicTestSort)

add_tdb_test(BasicTestCompositePlan)

add_library(tpch_testlib OBJECT
    TPCHTest.cpp
)
target_link_libraries(
    tpch_testlib
    tdb_test_base
)

# The system and bonus TPC-H tests are not added to ctest test list so that
# they are not run by default with ctest. These tests must be invoked directly
# using their executables.

configure_file("${CMAKE_SOURCE_DIR}/tests_scripts/RunTest.sh"
    "${CMAKE_BINARY_DIR}/tests/RunTest.sh" @ONLY)

