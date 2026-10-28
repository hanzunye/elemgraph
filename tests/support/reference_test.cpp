#include "reference.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace elemgraph::test {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(Reference, EmptyGraph) {
    const test_graph g{};
    EXPECT_THAT(connected_components(g), IsEmpty());
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, Path) {
    // 0 - 1 - 2 - 3
    const test_graph g{4, {{0, 1}, {1, 2}, {2, 3}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u, 0u, 0u));
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u, 2u, 3u));
    EXPECT_THAT(distances(g, 2), ElementsAre(2u, 1u, 0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u, 2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(1u, 2u));
}

TEST(Reference, Cycle) {
    const test_graph g{4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}};
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u, 2u, 1u));
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, Star) {
    const test_graph g{4, {{0, 1}, {2, 0}, {0, 3}}};
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u, 2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(0u));
}

TEST(Reference, TwoTrianglesSharingAVertex) {
    // 0 - 1     3 - 4
    //  \ /       \ /
    //   2 ------- 2   (the same vertex)
    const test_graph g{5, {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}}};
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), ElementsAre(2u));
}

TEST(Reference, ParallelEdgesAreNotBridges) {
    // Edges 0 and 1 both join 0 and 1, in opposite directions.
    const test_graph g{3, {{0, 1}, {1, 0}, {1, 2}}};
    EXPECT_THAT(bridges(g), ElementsAre(2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(1u));
}

TEST(Reference, SelfLoopsChangeNothing) {
    const test_graph g{2, {{0, 0}, {0, 1}, {1, 1}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u));
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(1u));
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, DisconnectedGraphWithIsolatedVertex) {
    // 0 - 1   2   3 - 4
    const test_graph g{5, {{0, 1}, {3, 4}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u, 1u, 2u, 2u));
    EXPECT_THAT(distances(g, 3), ElementsAre(none, none, none, 0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u));
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, CanonicalLabels) {
    const std::vector<std::uint32_t> labels{7, 7, 3, 9, 3};
    EXPECT_THAT(canonical_labels(labels), ElementsAre(0u, 0u, 1u, 2u, 1u));
    EXPECT_THAT(canonical_labels({}), IsEmpty());
}

}  // namespace
}  // namespace elemgraph::test
