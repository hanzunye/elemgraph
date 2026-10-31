#include "convert.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace elemgraph::test {
namespace {

using ::testing::ElementsAre;

TEST(ToEdgeList, KeepsTheVerticesAndTheOrderOfTheEdges) {
    const input_graph g{3, {{2, 0}, {1, 1}, {0, 1}}};
    const edge_list list = to_edge_list(g);
    EXPECT_EQ(list.num_vertices(), 3u);
    // Inside elemgraph::test, plain `edge` is the test type gen::edge.
    EXPECT_THAT(list.edges(),
                ElementsAre(elemgraph::edge{2, 0}, elemgraph::edge{1, 1}, elemgraph::edge{0, 1}));
}

}  // namespace
}  // namespace elemgraph::test
