# Graph inputs shared by benchmarks and tests: the plain graph type, random
# numbers, generators and readers. Included as "generators/<name>.hpp".
add_library(elemgraph_generators INTERFACE)
target_include_directories(elemgraph_generators INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(elemgraph_generators INTERFACE cxx_std_20)
