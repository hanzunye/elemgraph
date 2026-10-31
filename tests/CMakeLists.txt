find_package(GTest REQUIRED)
include(GoogleTest)

# One test binary for the whole library. Add new test files to this list.
# Files at the top level test the library in include/elemgraph/;
# support/ holds shared test code, each part with its own tests;
# generators/ tests the benchmark inputs in bench/generators/.
add_executable(elemgraph_tests
    csr_graph_random_test.cpp
    csr_graph_test.cpp
    edge_list_test.cpp
    generators/dimacs_test.cpp
    generators/input_graph_test.cpp
    generators/synthetic_test.cpp
    support/convert_test.cpp
    support/random_graphs_test.cpp
    support/reference_test.cpp
)
# Lets every test include shared test code as "support/<name>.hpp".
target_include_directories(elemgraph_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(elemgraph_tests PRIVATE
    elemgraph::elemgraph
    elemgraph_build_flags
    elemgraph_generators
    GTest::gmock_main
)

# Registers every TEST() in the binary as a separate CTest test.
gtest_discover_tests(elemgraph_tests)
