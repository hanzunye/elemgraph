#include "elemgraph/algorithms/components.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

// Runs both versions on both representations of `list` and checks that each
// of the four results is the expected one.
void expect_components(const edge_list& list, vertex count, const std::vector<vertex>& component) {
    const csr_graph csr(list);
    const adjacency_list adj(list);
    const auto check = [&](const std::string& version, const components_result& result) {
        SCOPED_TRACE(version);
        EXPECT_EQ(result.count, count);
        EXPECT_EQ(result.component, component);
    };
    check("bfs on csr_graph", connected_components_bfs(csr));
    check("bfs on adjacency_list", connected_components_bfs(adj));
    check("union-find on csr_graph", connected_components_union_find(csr));
    check("union-find on adjacency_list", connected_components_union_find(adj));
}

TEST(Components, EmptyGraph) {
    expect_components(edge_list(0), 0, {});
}

TEST(Components, VerticesWithoutEdgesAreComponentsOfTheirOwn) {
    expect_components(edge_list(3), 3, {0, 1, 2});
}

TEST(Components, AreNumberedInTheOrderOfTheirSmallestVertex) {
    edge_list list(6);
    list.add_edge(4, 1);
    list.add_edge(2, 5);
    list.add_edge(5, 0);  // components {0, 2, 5}, {1, 4}, {3}
    expect_components(list, 3, {0, 1, 0, 2, 1, 0});
}

TEST(Components, SelfLoopsAndParallelEdgesChangeNothing) {
    edge_list list(4);
    list.add_edge(3, 3);
    list.add_edge(0, 1);
    list.add_edge(1, 0);
    expect_components(list, 3, {0, 0, 1, 2});
}

TEST(Components, ReusedMemoryKeepsNothingFromTheLastGraph) {
    edge_list big(5);
    for (vertex v = 0; v + 1 < 5; ++v) {
        big.add_edge(v, v + 1);
    }
    edge_list small(3);
    small.add_edge(2, 0);

    components_result result;
    bfs_workspace workspace;
    union_find sets;
    connected_components_bfs(csr_graph(big), result, workspace);
    connected_components_bfs(csr_graph(small), result, workspace);
    EXPECT_EQ(result.count, 2u);
    EXPECT_EQ(result.component, (std::vector<vertex>{0, 1, 0}));
    connected_components_union_find(csr_graph(big), result, sets);
    connected_components_union_find(csr_graph(small), result, sets);
    EXPECT_EQ(result.count, 2u);
    EXPECT_EQ(result.component, (std::vector<vertex>{0, 1, 0}));
}

}  // namespace
}  // namespace elemgraph
