#include "elemgraph/edge_list.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>


namespace elemgraph {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(EdgeList, StartsWithVerticesButNoEdges) {
    const edge_list g(5);
    EXPECT_EQ(g.num_vertices(), 5u);
    EXPECT_EQ(g.num_edges(), 0u);
    EXPECT_THAT(g.edges(), IsEmpty());
}

TEST(EdgeList, NumbersEdgesInTheOrderTheyAreAdded) {
    edge_list g(3);
    EXPECT_EQ(g.add_edge(0, 1), 0u);
    EXPECT_EQ(g.add_edge(1, 2), 1u);
    EXPECT_EQ(g.add_edge(2, 0), 2u);
    EXPECT_EQ(g.num_edges(), 3u);
    EXPECT_THAT(g.edges(), ElementsAre(edge{0, 1}, edge{1, 2}, edge{2, 0}));
}

TEST(EdgeList, KeepsSelfLoopsAndParallelEdges) {
    edge_list g(2);
    EXPECT_EQ(g.add_edge(1, 1), 0u);
    EXPECT_EQ(g.add_edge(0, 1), 1u);
    EXPECT_EQ(g.add_edge(1, 0), 2u);
    EXPECT_EQ(g.num_edges(), 3u);
    EXPECT_THAT(g.edges(), ElementsAre(edge{1, 1}, edge{0, 1}, edge{1, 0}));
}

TEST(EdgeList, ReserveAddsNoEdges) {
    edge_list g(5);
    g.reserve(10);
    EXPECT_EQ(g.num_edges(), 0u);
    EXPECT_THAT(g.edges(), IsEmpty());
}

TEST(EdgeList, PrintsEdgesAsUDashV) {
    std::ostringstream os;
    os << edge{3, 7};
    EXPECT_EQ(os.str(), "3-7");
}

TEST(EdgeListDeathTest, AddEdgeRejectsVerticesOutOfRange) {
    edge_list g(3);
    EXPECT_DEBUG_DEATH(g.add_edge(0, 3), "");
    EXPECT_DEBUG_DEATH(g.add_edge(3, 0), "");
}

}  // namespace
}  // namespace elemgraph