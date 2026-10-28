// Tests use the graph type and the random numbers of the benchmark
// generators (bench/generators/), so a seed gives the same graph in tests and
// benchmarks.
#pragma once

#include "generators/input_graph.hpp"
#include "generators/rng.hpp"

namespace elemgraph::test {

using gen::edge;
using gen::edge_id;
using gen::input_graph;
using gen::rng;
using gen::vertex;

}  // namespace elemgraph::test
