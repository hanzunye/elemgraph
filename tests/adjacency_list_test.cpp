#include "elemgraph/adjacency_list.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

TEST(AdjacencyList, StoresEachEdgeAtBothEnds) {
    edge_list list(3);
    list.add_edge(0, 1);
    list.add_edge(1, 2);
    const adjacency_list g(list);

    EXPECT_EQ(g.num_vertices(), 3u);
    EXPECT_EQ(g.num_edges(), 2u);
    EXPECT_EQ(g.degree(1), 2u);
    EXPECT_THAT(g.neighbors(0), ElementsAre(1u));
    EXPECT_THAT(g.neighbors(1), ElementsAre(0u, 2u));
    EXPECT_THAT(g.neighbors(2), ElementsAre(1u));
}

TEST(AdjacencyList, EmptyGraph) {
    const adjacency_list g(edge_list(0));
    EXPECT_EQ(g.num_vertices(), 0u);
    EXPECT_EQ(g.num_edges(), 0u);
}

TEST(AdjacencyList, VerticesWithoutEdges) {
    const adjacency_list g(edge_list(3));
    EXPECT_EQ(g.num_vertices(), 3u);
    EXPECT_EQ(g.num_edges(), 0u);
    for (vertex v = 0; v < 3; ++v) {
        EXPECT_EQ(g.degree(v), 0u);
        EXPECT_THAT(g.neighbors(v), IsEmpty());
    }
}

TEST(AdjacencyList, DropsSelfLoopsAndKeepsParallelEdges) {
    edge_list list(3);
    list.add_edge(0, 1);  // id 0
    list.add_edge(1, 1);  // id 1: self-loop
    list.add_edge(1, 0);  // id 2: parallel to id 0
    list.add_edge(1, 2);  // id 3
    const adjacency_list g(list);

    EXPECT_EQ(g.num_edges(), 3u);
    EXPECT_THAT(g.neighbors(0), ElementsAre(1u, 1u));
    EXPECT_THAT(g.neighbors(1), ElementsAre(0u, 0u, 2u));
    EXPECT_THAT(g.neighbors(2), ElementsAre(1u));
}

TEST(AdjacencyList, HasTheSameNeighborsAsCsrGraph) {
    edge_list list(4);
    list.add_edge(0, 3);
    list.add_edge(0, 1);
    list.add_edge(2, 0);
    const adjacency_list a(list);
    const csr_graph c(list);

    for (vertex v = 0; v < 4; ++v) {
        EXPECT_THAT(a.neighbors(v), ElementsAreArray(c.neighbors(v))) << "vertex " << v;
    }
    EXPECT_THAT(a.neighbors(0), ElementsAre(3u, 1u, 2u));
}

}  // namespace
}  // namespace elemgraph
