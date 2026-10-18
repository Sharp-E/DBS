# src/query/CMakeLists.txt

set(EXECUTION_LIB_SRC
	TableScanState.cpp
    SelectionState.cpp
    ProjectionState.cpp
    TempTableState.cpp
    AggregationState.cpp
    SortState.cpp
    LimitState.cpp
)

add_tdb_object_library(execution ${EXECUTION_LIB_SRC})

