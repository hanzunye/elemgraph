// Checks both versions of connected components on both representations
// against the brute-force reference, on random multigraphs.
#include "elemgraph/algorithms/components.hpp"

#include <gtest/gtest.h>

#include <algorithm>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"
#include "support/reference.hpp"

namespace elemgraph::test {
namespace {

void expect_reference(const components_result& result, const input_graph& g) {
    // The reference numbers components in the order of their smallest vertex
    // too, so the labels must be equal, not just describe the same partition.
    const auto expected = connected_components(g);
    EXPECT_EQ(result.component, expected);
    const vertex expected_count =
        expected.empty() ? 0 : *std::max_element(expected.begin(), expected.end()) + 1;
    EXPECT_EQ(result.count, expected_count);
}

TEST(ComponentsRandom, AllVersionsMatchTheReference) {
    // Sparse multigraphs (up to 2n edges) have many components of all sizes.
    for_each_random_graph(1, 1000, small_random_multigraph, [](const input_graph& g) {
        const edge_list list = to_edge_list(g);
        const csr_graph csr(list);
        const adjacency_list adj(list);
        {
            SCOPED_TRACE("bfs on csr_graph");
            expect_reference(connected_components_bfs(csr), g);
        }
        {
            SCOPED_TRACE("bfs on adjacency_list");
            expect_reference(connected_components_bfs(adj), g);
        }
        {
            SCOPED_TRACE("union-find on csr_graph");
            expect_reference(connected_components_union_find(csr), g);
        }
        {
            SCOPED_TRACE("union-find on adjacency_list");
            expect_reference(connected_components_union_find(adj), g);
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
