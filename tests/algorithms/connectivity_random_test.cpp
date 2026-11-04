// Checks bridges and cut vertices against the brute-force references on
// random multigraphs, simple graphs and trees.
#include "elemgraph/algorithms/connectivity.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"
#include "support/reference.hpp"

namespace elemgraph::test {
namespace {

void expect_reference(const input_graph& g) {
    connectivity_result result = lowpoint_connectivity(csr_graph(to_edge_list(g)));
    std::sort(result.bridges.begin(), result.bridges.end());
    EXPECT_EQ(result.bridges, bridges(g));
    EXPECT_EQ(result.cut_vertices, cut_vertices(g));
}

TEST(LowpointRandom, MatchesTheReferenceOnMultigraphs) {
    for_each_random_graph(1, 1000, small_random_multigraph, expect_reference);
}

TEST(LowpointRandom, MatchesTheReferenceOnSimpleGraphs) {
    for_each_random_graph(1001, 1000, small_random_simple_graph, expect_reference);
}

TEST(LowpointRandom, EveryEdgeOfATreeIsABridge) {
    const auto make = [](rng& r) { return random_tree(r, static_cast<vertex>(r.below(40) + 1)); };
    for_each_random_graph(2001, 200, make, expect_reference);
}

}  // namespace
}  // namespace elemgraph::test
