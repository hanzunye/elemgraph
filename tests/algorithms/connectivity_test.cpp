#include "elemgraph/algorithms/connectivity.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

void expect_connectivity(const edge_list& list, std::vector<edge_id> bridges,
                         const std::vector<vertex>& cut_vertices) {
    connectivity_result result = lowpoint_connectivity(csr_graph(list));
    std::sort(result.bridges.begin(), result.bridges.end());
    std::sort(bridges.begin(), bridges.end());
    EXPECT_EQ(result.bridges, bridges);
    EXPECT_EQ(result.cut_vertices, cut_vertices);
}

edge_list path(vertex n) {
    edge_list list(n);
    for (vertex v = 0; v + 1 < n; ++v) {
        list.add_edge(v, v + 1);
    }
    return list;
}

TEST(Lowpoint, EmptyGraphAndSingleVertex) {
    expect_connectivity(edge_list(0), {}, {});
    expect_connectivity(edge_list(1), {}, {});
}

TEST(Lowpoint, SingleEdgeIsABridgeWithoutCutVertices) {
    expect_connectivity(path(2), {0}, {});
}

TEST(Lowpoint, PathHasOnlyBridgesAndInnerCutVertices) {
    expect_connectivity(path(4), {0, 1, 2}, {1, 2});
}

TEST(Lowpoint, CycleHasNeither) {
    edge_list list = path(5);
    list.add_edge(4, 0);
    expect_connectivity(list, {}, {});
}

TEST(Lowpoint, RootWithTwoChildrenIsACutVertex) {
    // Star with center 0, the root of the DFS: every edge is a bridge.
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(0, 2);
    list.add_edge(0, 3);
    expect_connectivity(list, {0, 1, 2}, {0});
}

TEST(Lowpoint, TwoTrianglesSharingAVertex) {
    edge_list list(5);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 0);
    list.add_edge(2, 3);
    list.add_edge(3, 4);
    list.add_edge(4, 2);
    expect_connectivity(list, {}, {2});
}

TEST(Lowpoint, ParallelEdgesAreNoBridgesButCanHangOnACutVertex) {
    // 0 = 1 - 2: the double edge 0-1 is no bridge, 1-2 is, and 1 is a cut
    // vertex because removing it separates 0 from 2.
    edge_list list(3);
    list.add_edge(0, 1);
    list.add_edge(1, 0);
    list.add_edge(1, 2);
    expect_connectivity(list, {2}, {1});
}

TEST(Lowpoint, SelfLoopsChangeNothing) {
    edge_list list(3);
    list.add_edge(1, 1);
    list.add_edge(0, 1);
    list.add_edge(0, 0);
    list.add_edge(1, 2);
    expect_connectivity(list, {1, 3}, {1});
}

TEST(Lowpoint, EveryComponentIsHandledOnItsOwn) {
    // Path 0-1-2, triangle 3-4-5, isolated 6, edge 7-8.
    edge_list list(9);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(3, 4);
    list.add_edge(4, 5);
    list.add_edge(5, 3);
    list.add_edge(7, 8);
    expect_connectivity(list, {0, 1, 5}, {1});
}

TEST(Lowpoint, BridgesAreOrderedByTheDfsNumberOfTheirLowerEnd) {
    // The path 0-1-2-3 with ids in the opposite order: from root 0, the
    // lower ends 1, 2, 3 carry the edges 2, 1, 0.
    edge_list list(4);
    list.add_edge(2, 3);
    list.add_edge(1, 2);
    list.add_edge(0, 1);
    const connectivity_result result = lowpoint_connectivity(csr_graph(list));
    EXPECT_EQ(result.bridges, (std::vector<edge_id>{2, 1, 0}));
}

TEST(Lowpoint, ReusedMemoryKeepsNothingFromTheLastGraph) {
    const csr_graph big(path(6));
    edge_list cycle = path(3);
    cycle.add_edge(2, 0);
    const csr_graph small(cycle);

    connectivity_result result;
    lowpoint_workspace workspace;
    lowpoint_connectivity(big, dfs(big), result, workspace);
    lowpoint_connectivity(small, dfs(small), result, workspace);
    EXPECT_TRUE(result.bridges.empty());
    EXPECT_TRUE(result.cut_vertices.empty());
}

}  // namespace
}  // namespace elemgraph
