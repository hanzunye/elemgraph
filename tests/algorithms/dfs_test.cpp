#include "elemgraph/algorithms/dfs.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

// Runs dfs() on both representations of `list` and checks the preorder and
// the parents; the numbers must be the positions in the preorder.
void expect_dfs(const edge_list& list, const std::vector<vertex>& order,
                const std::vector<vertex>& parent) {
    const auto check = [&](const std::string& representation, const dfs_result& result) {
        SCOPED_TRACE(representation);
        EXPECT_EQ(result.order, order);
        EXPECT_EQ(result.parent, parent);
        ASSERT_EQ(result.number.size(), order.size());
        for (vertex i = 0; i < order.size(); ++i) {
            EXPECT_EQ(result.number[order[i]], i) << "vertex " << order[i];
        }
    };
    check("csr_graph", dfs(csr_graph(list)));
    check("adjacency_list", dfs(adjacency_list(list)));
}

TEST(Dfs, EmptyGraph) {
    expect_dfs(edge_list(0), {}, {});
}

TEST(Dfs, Path) {
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 3);
    expect_dfs(list, {0, 1, 2, 3}, {no_vertex, 0, 1, 2});
}

TEST(Dfs, GoesDeepBeforeWide) {
    // Cycle 0-1-2-3-0. BFS would make 0 the parent of 3; DFS reaches 3 from 2.
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 3);
    list.add_edge(3, 0);
    expect_dfs(list, {0, 1, 2, 3}, {no_vertex, 0, 1, 2});
}

TEST(Dfs, FollowsTheEdgesOfAVertexInIdOrder) {
    // The neighbors of 0 are 2 (edge 0) and then 1 (edge 1), so the subtree
    // of 2 comes before 1 in the preorder.
    edge_list list(4);
    list.add_edge(0, 2);
    list.add_edge(0, 1);
    list.add_edge(2, 3);
    expect_dfs(list, {0, 2, 3, 1}, {no_vertex, 0, 0, 2});
}

TEST(Dfs, EachComponentIsATreeRootedAtItsSmallestVertex) {
    edge_list list(6);
    list.add_edge(4, 1);
    list.add_edge(2, 5);
    list.add_edge(5, 0);  // components {0, 2, 5}, {1, 4}, {3}
    expect_dfs(list, {0, 5, 2, 1, 4, 3}, {no_vertex, no_vertex, 5, no_vertex, 1, 0});
}

TEST(Dfs, SelfLoopsAndParallelEdgesChangeNothing) {
    edge_list list(3);
    list.add_edge(0, 0);
    list.add_edge(0, 1);
    list.add_edge(1, 0);
    list.add_edge(1, 2);
    expect_dfs(list, {0, 1, 2}, {no_vertex, 0, 1});
}

TEST(Dfs, LongPathDoesNotOverflowTheCallStack) {
    const vertex n = 1'000'000;
    edge_list list(n);
    for (vertex v = 0; v + 1 < n; ++v) {
        list.add_edge(v, v + 1);
    }
    const dfs_result result = dfs(csr_graph(list));
    ASSERT_EQ(result.order.size(), n);
    EXPECT_EQ(result.order.back(), n - 1);
    EXPECT_EQ(result.parent[n - 1], n - 2);
}

TEST(Dfs, ReusedResultAndWorkspaceKeepNothingFromTheLastSearch) {
    edge_list big(5);
    for (vertex v = 0; v + 1 < 5; ++v) {
        big.add_edge(v, v + 1);
    }
    edge_list small(3);
    small.add_edge(2, 1);

    dfs_result result;
    dfs_workspace workspace;
    dfs(csr_graph(big), result, workspace);
    dfs(csr_graph(small), result, workspace);
    EXPECT_EQ(result.order, (std::vector<vertex>{0, 1, 2}));
    EXPECT_EQ(result.number, (std::vector<vertex>{0, 1, 2}));
    EXPECT_EQ(result.parent, (std::vector<vertex>{no_vertex, no_vertex, 1}));
}

}  // namespace
}  // namespace elemgraph
