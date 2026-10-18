# tests/storage/CMakeLists.txt

add_library(extsort_testlib OBJECT
    PseudoRandomItemIterator.cpp
)
target_link_libraries(extsort_testlib tdb_test_base)

add_tdb_test_binary(BasicTestExternalSort)
target_link_libraries(BasicTestExternalSort extsort_testlib)

set(EXTSORT_TEST_LIST
    BasicTestExternalSort.TestSortUniformInt64Asc
    BasicTestExternalSort.TestSortUniformInt64Desc
    BasicTestExternalSort.TestSortUniformStringAsc
    BasicTestExternalSort.TestSortUniformStringDesc
)

set(EXTSORT_TEST_INT_TIMEOUT 4s)
set(EXTSORT_TEST_STRING_TIMEOUT 10s)

foreach (t IN LISTS EXTSORT_TEST_LIST)
    configure_file("${CMAKE_SOURCE_DIR}/tests_scripts/extsort/${t}.sh"
        "${CMAKE_BINARY_DIR}/tests/extsort/${t}.sh" @ONLY)
    add_test(
        NAME ${t}
        COMMAND "${CMAKE_BINARY_DIR}/tests/extsort/${t}.sh"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
