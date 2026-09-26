# CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

find_package(PkgConfig REQUIRED)
include(CheckCXXSourceCompiles)
include(CheckSymbolExists)

project(taco-db
    VERSION 0.1.0
    LANGUAGES C CXX)

option(USE_JEMALLOC
    "whether to use jemalloc(3) in place of malloc(3)"
    ON)

# set default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  message(NOTICE "-- No build mode chosen - using Debug by default")
endif(NOT CMAKE_BUILD_TYPE)

# find a few programs

find_program(SHELL bash)
if(NOT SHELL)
    message(FATAL_ERROR "can't find bash")
endif()

find_program(SED sed)
if (NOT SED)
    message(FATAL_ERROR "can't find sed")
endif()

find_program(GREP grep)
if (NOT GREP)
    message(FATAL_ERROR "can't find grep")
endif()

# c++ compiler settings

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
## disable strict aliasing
if("${CMAKE_BUILD_TYPE}x" STREQUAL "Debugx")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g2 -Wall -Wextra -Wno-unused-parameter -fno-omit-frame-pointer")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
## disable warning about missing fields in an aggregate initializer
## This will avoid the warnings about the usage like this:
## struct A { int x, y, z; };
## A a{.x = 1, .y = 2}; // default initializes a.z to 0
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-unused-private-field -Wno-unknown-warning-option")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # suppresses warning at absl/functional/function_ref.h:124 at 278e0a0
    # (20210324.2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-copy")
endif()

# check if -E option works for the c++ compiler
execute_process(
    COMMAND bash -c "echo 'FOO' | ${CMAKE_CXX_COMPILER} -E -DFOO=BAR -"
    RESULT_VARIABLE CPP_TEST_RES
    OUTPUT_VARIABLE CPP_TEST_OUT)
set(CPP_FOUND 0)
if("${CPP_TEST_RES}" EQUAL 0)
    string(FIND "${CPP_TEST_OUT}" "FOO" CPP_TEST_POS)
    if("${CPP_TEST_POS}" EQUAL -1)
        set(CPP_FOUND 1)
    endif()
endif()
if(NOT CPP_FOUND)
    message(FATAL_ERROR
        "${CMAKE_CXX_COMPILER} -E does not work as a preprocessor")
endif()

# Check and configure external libraries

# Check if -latomic is needed
file(READ ${PROJECT_SOURCE_DIR}/cmake/check_libatomic.cpp CHECK_LIBATOMIC_SRC)
check_cxx_source_compiles("${CHECK_LIBATOMIC_SRC}" NO_LIBATOMIC_NEEDED)
if (NOT NO_LIBATOMIC_NEEDED)
    # see https://github.com/HewlettPackard/foedus_code/blob/master/foedus-core/cmake/FindGccAtomic.cmake
    find_library(LIBATOMIC NAMES atomic atomic.so.1 libatomic.so.1)
    if (NOT LIBATOMIC)
        message(FATAL_ERROR "unable to find libatomic")
    endif()
    message(${LIBATOMIC})
    link_libraries(${LIBATOMIC})
    check_cxx_source_compiles("${CHECK_LIBATOMIC_SRC}" LIBATOMIC_OK)
    if (NOT LIBATOMIC_OK)
        message(FATAL_ERROR "libatomic does not work")
    endif()
endif()

# Check if <atomic> satisfies our requirements
file(READ ${PROJECT_SOURCE_DIR}/cmake/check_atomic.cpp CHECK_ATOMIC_SRC)
set(CMAKE_REQUIRED_LIBRARIES ${LIBATOMIC})
check_cxx_source_compiles("${CHECK_ATOMIC_SRC}" ATOMIC_OK)
if (NOT ATOMIC_OK)
    message(FATAL_ERROR "The <atomic> library does not satisfy the requirement")
endif()

# check if we have fallocate(2)
file(READ ${PROJECT_SOURCE_DIR}/cmake/check_fallocate.cpp CHECK_FALLOCATE_SRC)
check_cxx_source_compiles("${CHECK_FALLOCATE_SRC}" HAVE_FALLOCATE)

# check if we have strncasecmp(3)
check_symbol_exists(strncasecmp "strings.h" HAVE_STRNCASECMP)
if (NOT HAVE_STRNCASECMP)
    message(FATAL_ERROR "strncasecmp(3) not found")
endif()

# build and load jemalloc if needed
if(USE_JEMALLOC)
    pkg_check_modules(JEMALLOC REQUIRED jemalloc)
    include_directories(${JEMALLOC_INCLUDEDIR})
    link_libraries(-L${JEMALLOC_LIBDIR} -l${JEMALLOC_LIBRARIES} -Wl,-rpath,${JEMALLOC_LIBDIR})
endif()

# adding pthread, add this before absl and gtest
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# need to enable testing before adding GoogleTest
enable_testing()

# This is a list of the names (without the absl:: or absl_ prefix) of the absl
# target we directly use.
set(ABSL_TARGET_NAMES
    log_severity
    flat_hash_map
    flat_hash_set
    flags
    flags_parse
    flags_usage
    memory
    strings
    str_format
)

find_package(GTest CONFIG)
if (GTest_FOUND)
    get_target_property(GTest_is_pic GTest::gtest POSITION_INDEPENDENT_CODE)
    if (NOT GTest_is_pic)
        message(FATAL_ERROR "GoogleTest found at the default location is not a shared lib")
    endif()
else()
    message(FATAL_ERROR "unable to find GoogleTest")
endif()

find_package(absl CONFIG)
if (absl_FOUND)
    get_target_property(absl_is_pic absl::base POSITION_INDEPENDENT_CODE)
    if (NOT absl_is_pic)
        message(FATAL_ERROR "Abseil found at the default location is not a shared lib, skipped")
    endif()
else()
    message(FATAL_ERROR "unable to find Abseil")
endif()

list(TRANSFORM ABSL_TARGET_NAMES PREPEND absl::
    OUTPUT_VARIABLE ABSL_TARGETS)
foreach(abslTarget IN LISTS ${ABSL_MODULE_NAMES})
    link_libraries(${abslTarget})
endforeach()
link_libraries(${ABSL_TARGETS})
link_libraries(GTest::gtest)
link_libraries(GTest::gmock)

# adding the GoogleTest module for gtest_add_test()
include(GoogleTest)

# Taco-DB project configuration
execute_process(
    COMMAND getconf LEVEL1_DCACHE_LINESIZE
    OUTPUT_VARIABLE CONFIG_L1_CACHELINE_SIZE
    RESULT_VARIABLE EXIT_CODE)
if (NOT ${EXIT_CODE} EQUAL 0)
    # l1 cache line size defaults to 64 bytes if getconf is N/A
    set(CONFIG_L1_CACHELINE_SIZE 64)
endif()

## Configurations

# 4KB page
# XXX This is currently fixed as many places assume 4KB page.
set(CONFIG_PAGE_SIZE 4096)

# don't force fsync by default
set(FORCE_FSYNC 0)

if (NOT DEFINED USE_VOLATILE_CATCACHE)
    # defaults to use volatile catalog cache before we have heap file
    set(USE_VOLATILE_CATCACHE ON)
endif()

if (USE_VOLATILE_CATCACHE)
set(CATALOG_CACHE_HEADER "catalog/VolatileCatCache.h")
set(CATALOG_CACHE_CLASS "::taco::VolatileCatCache")
else()
message(FATAL_ERROR
    "can't use persistent catalog before we have a heap file implementation")
endif()

set(ALWAYS_USE_FIXEDLEN_DATAPAGE OFF)
set(ALWAYS_USE_VARLEN_DATAPAGE ON)

if (ALWAYS_USE_FIXEDLEN_DATAPAGE AND ALWAYS_USE_VARLEN_DATAPAGE)
    message(FATAL_ERROR
    "at most one of ALWAYS_USE_FIXEDLEN_DATAPAGE and ALWAYS_USE_VARLEN_DTAPAGE may be set to ON")
endif()

set(GENERATED_SOURCE_DIR ${CMAKE_BINARY_DIR}/generated_source)

# the temporary directory for test data
set(TEST_TMPDIR ${CMAKE_BINARY_DIR}/tmp)
file(MAKE_DIRECTORY "${TEST_TMPDIR}")
execute_process(
    COMMAND bash -c "chmod 0700 ${TEST_TMPDIR}"
    RESULT_VARIABLE CHMOD_RES
)
if(CHMOD_RES)
    message(FATAL_ERROR "unable to change the permission of ${TEST_TMPDIR} to 0700")
endif()

configure_file(config.h.in
	${GENERATED_SOURCE_DIR}/config.h)
configure_file(include/catalog/CatCache.h.in
    ${GENERATED_SOURCE_DIR}/catalog/CatCache.h)
include_directories("${GENERATED_SOURCE_DIR}" include)

## Targets

### The target for all generated source code (except config.h).
add_custom_target(AllGeneratedSource)

### subdirectories
add_subdirectory(src)

### add unit tests
add_subdirectory(tests)

