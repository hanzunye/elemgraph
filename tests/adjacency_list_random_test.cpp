// Checks adjacency_list against csr_graph on random multigraphs.
#include "elemgraph/adjacency_list.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"

namespace elemgraph::test {
namespace {

using ::testing::ElementsAreArray;

// csr_graph_random_test.cpp checks csr_graph against the edge list, so here
// csr_graph serves as the reference.
TEST(AdjacencyListRandom, HasTheSameNeighborsAsCsrGraph) {
    for_each_random_graph(1, 1000, small_random_multigraph, [](const input_graph& g) {
        const edge_list list = to_edge_list(g);
        const adjacency_list adj(list);
        const csr_graph csr(list);
        ASSERT_EQ(adj.num_vertices(), csr.num_vertices());
        EXPECT_EQ(adj.num_edges(), csr.num_edges());
        for (vertex v = 0; v < csr.num_vertices(); ++v) {
            EXPECT_EQ(adj.degree(v), csr.degree(v)) << "vertex " << v;
            EXPECT_THAT(adj.neighbors(v), ElementsAreArray(csr.neighbors(v))) << "vertex " << v;
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
