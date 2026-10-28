find_package(GTest REQUIRED)
include(GoogleTest)

# One test binary for the whole library. Add new test files to this list.
add_executable(elemgraph_tests
    smoke_test.cpp
)
target_link_libraries(elemgraph_tests PRIVATE
    elemgraph::elemgraph
    elemgraph_build_flags
    GTest::gtest_main
)

# Registers every TEST() in the binary as a separate CTest test.
gtest_discover_tests(elemgraph_tests)
