#include "elemgraph/csr_graph.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace elemgraph {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(CsrGraph, VerticesWithoutEdges) {
    const csr_graph g(edge_list(3));
    EXPECT_EQ(g.num_vertices(), 3u);
    EXPECT_EQ(g.num_edges(), 0u);
    for (vertex v = 0; v < 3; ++v) {
        EXPECT_EQ(g.degree(v), 0u);
        EXPECT_THAT(g.neighbors(v), IsEmpty());
        EXPECT_THAT(g.edge_ids(v), IsEmpty());
    }
}

TEST(CsrGraph, StoresEachEdgeAtBothEndsUnderTheSameId) {
    edge_list list(3);
    list.add_edge(0, 1);  // id 0
    list.add_edge(1, 2);  // id 1
    const csr_graph g(list);

    EXPECT_EQ(g.num_edges(), 2u);
    EXPECT_EQ(g.degree(1), 2u);
    EXPECT_THAT(g.neighbors(0), ElementsAre(1u));
    EXPECT_THAT(g.edge_ids(0), ElementsAre(0u));
    EXPECT_THAT(g.neighbors(1), ElementsAre(0u, 2u));
    EXPECT_THAT(g.edge_ids(1), ElementsAre(0u, 1u));
    EXPECT_THAT(g.neighbors(2), ElementsAre(1u));
    EXPECT_THAT(g.edge_ids(2), ElementsAre(1u));
}

TEST(CsrGraph, EmptyGraph) {
    const csr_graph g(edge_list(0));
    EXPECT_EQ(g.num_vertices(), 0u);
    EXPECT_EQ(g.num_edges(), 0u);
}

TEST(CsrGraph, OrdersTheEdgesOfEachVertexById) {
    edge_list list(4);
    list.add_edge(0, 3);  // id 0
    list.add_edge(0, 1);  // id 1
    list.add_edge(2, 0);  // id 2
    const csr_graph g(list);
    EXPECT_THAT(g.neighbors(0), ElementsAre(3u, 1u, 2u));
    EXPECT_THAT(g.edge_ids(0), ElementsAre(0u, 1u, 2u));
}

TEST(CsrGraph, DropsSelfLoops) {
    edge_list list(3);
    list.add_edge(0, 1);  // id 0
    list.add_edge(1, 1);  // id 1: self-loop
    list.add_edge(1, 2);  // id 2
    const csr_graph g(list);
    EXPECT_EQ(g.num_edges(), 2u);
    EXPECT_THAT(g.neighbors(1), ElementsAre(0u, 2u));
    EXPECT_THAT(g.edge_ids(1), ElementsAre(0u, 2u));
}

TEST(CsrGraph, KeepsParallelEdges) {
    edge_list list(2);
    list.add_edge(0, 1);  // id 0
    list.add_edge(1, 0);  // id 1: parallel edge
    const csr_graph g(list);
    EXPECT_EQ(g.num_edges(), 2u);
    EXPECT_EQ(g.degree(0), 2u);
    EXPECT_EQ(g.degree(1), 2u);
    EXPECT_THAT(g.neighbors(0), ElementsAre(1u, 1u));
    EXPECT_THAT(g.edge_ids(0), ElementsAre(0u, 1u));
    EXPECT_THAT(g.neighbors(1), ElementsAre(0u, 0u));
    EXPECT_THAT(g.edge_ids(1), ElementsAre(0u, 1u));
}

}  // namespace
}  // namespace elemgraph