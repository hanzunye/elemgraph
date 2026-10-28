find_package(GTest REQUIRED)
include(GoogleTest)

# One test binary for the whole library. Add new test files to this list.
# support/ holds shared test code, each part with its own tests.
add_executable(elemgraph_tests
    support/random_graphs_test.cpp
    support/reference_test.cpp
)
target_link_libraries(elemgraph_tests PRIVATE
    elemgraph::elemgraph
    elemgraph_build_flags
    elemgraph_generators
    GTest::gmock_main
)

# Registers every TEST() in the binary as a separate CTest test.
gtest_discover_tests(elemgraph_tests)
