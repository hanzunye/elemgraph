# Graph inputs shared by benchmarks and tests: the plain graph type, random
# numbers, generators and readers. Included as "generators/<name>.hpp".
add_library(elemgraph_generators INTERFACE)
target_include_directories(elemgraph_generators INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(elemgraph_generators INTERFACE cxx_std_20)

# One benchmark binary; each suite is a separate file. See main.cpp for how
# to run it.
find_package(benchmark REQUIRED)
add_executable(elemgraph_bench
    bfs.cpp
    connectivity.cpp
    main.cpp
)
target_link_libraries(elemgraph_bench PRIVATE
    elemgraph::elemgraph
    elemgraph_build_flags
    elemgraph_generators
    benchmark::benchmark
)

# Runs every benchmark once on tiny graphs, so that ctest (and with it CI)
# notices when a benchmark no longer builds or crashes.
add_test(NAME bench_smoke COMMAND elemgraph_bench --smoke --benchmark_min_time=1x)
