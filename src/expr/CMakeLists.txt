# src/expr/CMakeLists.txt

set(EXPR_LIB_SRC
    optypes.cpp
    )

add_tdb_object_library(expr ${EXPR_LIB_SRC})

