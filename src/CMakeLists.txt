# src/CMakeLists.txt

# empty list for the subdirectorie's CMakeLists.txt to append to
set_property(GLOBAL PROPERTY LIBTDB_LINK_OBJLIBS_PROP)

function(add_tdb_object_library libname)

    add_library(${libname}_test OBJECT ${ARGN})
    add_dependencies(${libname}_test AllGeneratedSource)
    target_compile_definitions(${libname}_test PUBLIC -DTDB_IN_TEST)
    set_property(TARGET ${libname}_test PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(GLOBAL APPEND PROPERTY LIBTDB_LINK_OBJLIBS_PROP ${libname})
endfunction()

# collect all build targets

add_subdirectory(base)

add_subdirectory(dbmain)

add_subdirectory(expr)

add_subdirectory(storage)

add_subdirectory(utils)

add_subdirectory(index)

## catalog is added at the very end so that we can get the list of files
## with builtin functions in FILES_WITH_BUILTIN_FUNCS set in
## utils/CMakeLists.txt
add_subdirectory(catalog)

# libtdb.a is built for non-test programs
get_property(LIBTDB_LINK_OBJLIBS GLOBAL PROPERTY LIBTDB_LINK_OBJLIBS_PROP)

set(LIBTDB_LINK_EXTLIBS
    Threads::Threads
    ${ABSL_TARGET})
if (CONFIG_USE_JEMALLOC)
    list(APPEND LIBTDB_LINK_EXTLIBS ${JEMALLOC_LIBRARIES})
endif()

set(LIBTDB_LINK_OPTIONS
    -Wl,-u,FLAGS_sort_nways
    -Wl,-u,FLAGS_test_never_call_fallocate
    -Wl,-u,FLAGS_btree_enable_rebalancing_pages
)

# libtdb_test.a is built for test programs
# The difference is that libtdb_test.a has a different way of printing logs
# and assertion errors
list(TRANSFORM LIBTDB_LINK_OBJLIBS APPEND "_test"
    OUTPUT_VARIABLE LIBTDB_TEST_LINK_OBJLIBS)
set(LIBTDB_TEST_LINK_EXTLIBS ${LIBTDB_LINK_EXTLIBS})

add_library(tdb_test SHARED)
target_link_libraries(tdb_test ${LIBTDB_TEST_LINK_OBJLIBS})
target_link_libraries(tdb_test ${LIBTDB_TEST_LINK_EXTLIBS})
target_link_options(tdb_test BEFORE INTERFACE ${LIBTDB_LINK_OPTIONS})
target_link_options(tdb_test BEFORE INTERFACE "-Wl,--disable-new-dtags")
#target_compile_definitions(tdb_test PUBLIC -DTDB_IN_TEST)


# We still need to build the static library for wrapping
# taco_fileman_readpage_impl and taco_fileman_writepage_impl during linking for
# the buffer manager and eviction tests.
add_library(tdb_test_static STATIC)
target_link_libraries(tdb_test_static ${LIBTDB_TEST_LINK_OBJLIBS})
target_link_libraries(tdb_test_static ${LIBTDB_TEST_LINK_EXTLIBS})
target_link_options(tdb_test_static BEFORE INTERFACE ${LIBTDB_LINK_OPTIONS})
target_link_options(tdb_test_static BEFORE INTERFACE "-Wl,--disable-new-dtags")


add_custom_target(remove_old_tmp_files
	bash ${CMAKE_SOURCE_DIR}/remove_old_tmp_files.sh "${CMAKE_BINARY_DIR}/tmp"
)

add_dependencies(tdb_test remove_old_tmp_files)

