#include "elemgraph/algorithms/bfs.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"
#include "generators/synthetic.hpp"
#include "support/convert.hpp"

namespace elemgraph {
namespace {

// Runs both BFS versions on both representations of `list` and checks that
// each of the four results is the expected one.
void expect_bfs(const edge_list& list, vertex source, const std::vector<vertex>& distance,
                const std::vector<vertex>& parent) {
    const csr_graph csr(list);
    const adjacency_list adj(list);
    const auto check = [&](const std::string& version, const bfs_result& result) {
        SCOPED_TRACE(version);
        EXPECT_EQ(result.distance, distance);
        EXPECT_EQ(result.parent, parent);
    };
    check("bfs on csr_graph", bfs(csr, source));
    check("bfs on adjacency_list", bfs(adj, source));
    check("bfs_std_queue on csr_graph", bfs_std_queue(csr, source));
    check("bfs_std_queue on adjacency_list", bfs_std_queue(adj, source));
}

TEST(Bfs, SingleVertex) {
    expect_bfs(edge_list(1), 0, {0}, {no_vertex});
}

TEST(Bfs, Path) {
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 3);
    expect_bfs(list, 1, {1, 0, 1, 2}, {1, no_vertex, 1, 2});
}

TEST(Bfs, CycleIsSearchedInEdgeIdOrder) {
    // 0-1-2-3-4-0. The neighbors of 0 are 1 (edge 0) and then 4 (edge 4), so 1
    // enters the queue first and becomes the parent of 2; 4 becomes the
    // parent of 3, although 3 is also a neighbor of 2.
    edge_list list(5);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 3);
    list.add_edge(3, 4);
    list.add_edge(4, 0);
    expect_bfs(list, 0, {0, 1, 2, 2, 1}, {no_vertex, 0, 1, 4, 0});
}

TEST(Bfs, VerticesInOtherComponentsAreUnreachable) {
    edge_list list(5);
    list.add_edge(0, 1);
    list.add_edge(2, 3);  // vertex 4 has no edges
    expect_bfs(list, 0, {0, 1, unreachable, unreachable, unreachable},
               {no_vertex, 0, no_vertex, no_vertex, no_vertex});
}

TEST(Bfs, SelfLoopsAndParallelEdgesChangeNothing) {
    edge_list list(3);
    list.add_edge(0, 0);
    list.add_edge(0, 1);
    list.add_edge(1, 0);
    list.add_edge(1, 2);
    list.add_edge(2, 2);
    expect_bfs(list, 0, {0, 1, 2}, {no_vertex, 0, 1});
}

TEST(Bfs, GridDistancesAreManhattanDistances) {
    const vertex rows = 30;
    const vertex cols = 40;
    const csr_graph g(test::to_edge_list(gen::grid(rows, cols)));
    const bfs_result result = bfs(g, 0);
    for (vertex r = 0; r < rows; ++r) {
        for (vertex c = 0; c < cols; ++c) {
            ASSERT_EQ(result.distance[r * cols + c], r + c) << "row " << r << ", column " << c;
        }
    }
}

TEST(Bfs, ReusedResultAndWorkspaceKeepNothingFromTheLastSearch) {
    edge_list big(5);
    for (vertex v = 0; v + 1 < 5; ++v) {
        big.add_edge(v, v + 1);
    }
    edge_list small(3);
    small.add_edge(1, 2);

    bfs_result result;
    bfs_workspace workspace;
    bfs(csr_graph(big), 0, result, workspace);
    bfs(csr_graph(small), 1, result, workspace);
    EXPECT_EQ(result.distance, (std::vector<vertex>{unreachable, 0, 1}));
    EXPECT_EQ(result.parent, (std::vector<vertex>{no_vertex, no_vertex, 1}));
}

}  // namespace
}  // namespace elemgraph
