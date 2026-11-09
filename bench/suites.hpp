// Each suite registers its benchmarks for the given sizes (log2 of the number
// of vertices). main.cpp decides the sizes.
#pragma once

#include <span>

namespace elemgraph::bench {

// Q1 for BFS, and the flat-array queue against std::queue: bfs.cpp.
void register_bfs(std::span<const unsigned> log_sizes);

// Q4, the lowpoint algorithm against the chain decomposition: connectivity.cpp.
void register_connectivity(std::span<const unsigned> log_sizes);

}  // namespace elemgraph::bench
