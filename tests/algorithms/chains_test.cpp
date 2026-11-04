#include "elemgraph/algorithms/connectivity.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

using ::testing::ElementsAre;

chain_decomposition chains_of(const csr_graph& g, connectivity_result& result) {
    chain_workspace workspace;
    chain_decomposition chains;
    chain_connectivity(g, dfs(g), result, workspace, chains);
    return chains;
}

TEST(Chains, TriangleIsOneCycle) {
    // DFS 0 -> 1 -> 2; the back edge 2-0 (id 2) gives the only chain.
    edge_list list(3);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 0);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    ASSERT_EQ(chains.size(), 1u);
    EXPECT_THAT(chains.vertices(0), ElementsAre(0u, 2u, 1u, 0u));
    EXPECT_THAT(chains.edges(0), ElementsAre(2u, 1u, 0u));
    EXPECT_TRUE(chains.is_cycle(0));
    EXPECT_TRUE(result.bridges.empty());
    EXPECT_TRUE(result.cut_vertices.empty());
}

TEST(Chains, SecondCycleStartsAtACutVertex) {
    // Two triangles sharing vertex 2. DFS 0 -> 1 -> 2 -> 3 -> 4. C_1 is the
    // cycle from 0; the second cycle starts at 2, so 2 is a cut vertex.
    edge_list list(5);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 0);
    list.add_edge(2, 3);
    list.add_edge(3, 4);
    list.add_edge(4, 2);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    ASSERT_EQ(chains.size(), 2u);
    EXPECT_THAT(chains.vertices(0), ElementsAre(0u, 2u, 1u, 0u));
    EXPECT_THAT(chains.vertices(1), ElementsAre(2u, 4u, 3u, 2u));
    EXPECT_THAT(chains.edges(1), ElementsAre(5u, 4u, 3u));
    EXPECT_TRUE(result.bridges.empty());
    EXPECT_THAT(result.cut_vertices, ElementsAre(2u));
}

TEST(Chains, EarIsAPathBetweenVisitedVertices) {
    // Cycle 0-1-2-3-0 with the chord 1-3. DFS 0 -> 1 -> 2 -> 3. From 0: the
    // cycle 0, 3, 2, 1, 0. From 1: the chord alone, a path from 1 to 3.
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 3);
    list.add_edge(3, 0);
    list.add_edge(1, 3);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    ASSERT_EQ(chains.size(), 2u);
    EXPECT_THAT(chains.vertices(0), ElementsAre(0u, 3u, 2u, 1u, 0u));
    EXPECT_THAT(chains.vertices(1), ElementsAre(1u, 3u));
    EXPECT_THAT(chains.edges(1), ElementsAre(4u));
    EXPECT_FALSE(chains.is_cycle(1));
    EXPECT_TRUE(result.cut_vertices.empty());
}

TEST(Chains, ParallelEdgeIsACycleOfLengthTwo) {
    // 0 - 1 = 2: the second edge between 1 and 2 is a back edge, and its
    // chain 1, 2, 1 is the C_1 of the graph. 0-1 lies in no chain: a bridge.
    edge_list list(3);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 1);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    ASSERT_EQ(chains.size(), 1u);
    EXPECT_THAT(chains.vertices(0), ElementsAre(1u, 2u, 1u));
    EXPECT_THAT(chains.edges(0), ElementsAre(2u, 1u));
    EXPECT_THAT(result.bridges, ElementsAre(0u));
    EXPECT_THAT(result.cut_vertices, ElementsAre(1u));
}

TEST(Chains, TreeHasNoChainsAndOnlyBridges) {
    edge_list list(4);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(1, 3);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    EXPECT_EQ(chains.size(), 0u);
    EXPECT_THAT(result.bridges, ElementsAre(0u, 1u, 2u));
    EXPECT_THAT(result.cut_vertices, ElementsAre(1u));  // 0, 2 and 3 have degree 1
}

TEST(Chains, EachComponentHasItsOwnFirstCycle) {
    // Two disjoint triangles: each one's cycle is the C_1 of its component,
    // so neither start is a cut vertex.
    edge_list list(6);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    list.add_edge(2, 0);
    list.add_edge(3, 4);
    list.add_edge(4, 5);
    list.add_edge(5, 3);
    connectivity_result result;
    const chain_decomposition chains = chains_of(csr_graph(list), result);
    ASSERT_EQ(chains.size(), 2u);
    EXPECT_TRUE(chains.is_cycle(0));
    EXPECT_TRUE(chains.is_cycle(1));
    EXPECT_TRUE(result.cut_vertices.empty());
}

TEST(Chains, ReusedMemoryKeepsNothingFromTheLastGraph) {
    edge_list two_triangles(5);
    two_triangles.add_edge(0, 1);
    two_triangles.add_edge(1, 2);
    two_triangles.add_edge(2, 0);
    two_triangles.add_edge(2, 3);
    two_triangles.add_edge(3, 4);
    two_triangles.add_edge(4, 2);
    edge_list path(3);
    path.add_edge(0, 1);
    path.add_edge(1, 2);
    const csr_graph big(two_triangles);
    const csr_graph small(path);

    connectivity_result result;
    chain_workspace workspace;
    chain_decomposition chains;
    chain_connectivity(big, dfs(big), result, workspace, chains);
    chain_connectivity(small, dfs(small), result, workspace, chains);
    EXPECT_EQ(chains.size(), 0u);
    EXPECT_THAT(result.bridges, ElementsAre(0u, 1u));
    EXPECT_THAT(result.cut_vertices, ElementsAre(1u));
}

}  // namespace
}  // namespace elemgraph
