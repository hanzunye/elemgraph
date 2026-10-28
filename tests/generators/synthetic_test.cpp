#include "generators/synthetic.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "support/reference.hpp"

namespace elemgraph::gen {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(Grid, HasNoBridges) {
    // 3 rows x 4 columns: 3 * 3 horizontal and 2 * 4 vertical edges.
    const input_graph g = grid(3, 4);
    EXPECT_EQ(g.n, 12u);
    EXPECT_EQ(g.edges.size(), 17u);
    EXPECT_TRUE(is_canonical(g));
    EXPECT_EQ(test::connected_components(g), std::vector<std::uint32_t>(12, 0));
    EXPECT_THAT(test::bridges(g), IsEmpty());
}

TEST(Grid, NumbersVerticesRowByRow) {
    // 0 - 1 - 2
    // |   |   |
    // 3 - 4 - 5
    EXPECT_EQ(grid(2, 3).edges,
              (std::vector<edge>{{0, 1}, {0, 3}, {1, 2}, {1, 4}, {2, 5}, {3, 4}, {4, 5}}));
}

TEST(Grid, OneRowIsAPath) {
    const input_graph g = grid(1, 5);
    EXPECT_EQ(g.edges, (std::vector<edge>{{0, 1}, {1, 2}, {2, 3}, {3, 4}}));
    EXPECT_THAT(test::bridges(g), ElementsAre(0u, 1u, 2u, 3u));
}

TEST(Grid, Empty) {
    EXPECT_EQ(grid(0, 7), input_graph{});
    EXPECT_EQ(grid(1, 1), (input_graph{1, {}}));
}

TEST(Gnm, HasExactlyMDistinctEdges) {
    const input_graph g = gnm(1000, 5000, 1);
    EXPECT_EQ(g.n, 1000u);
    EXPECT_EQ(g.edges.size(), 5000u);
    EXPECT_TRUE(is_canonical(g));
}

TEST(Gnm, CanBeComplete) {
    const input_graph g = gnm(20, 190, 1);
    EXPECT_EQ(g.edges.size(), 190u);
    EXPECT_TRUE(is_canonical(g));
}

TEST(Gnm, SeedSelectsTheGraph) {
    EXPECT_EQ(gnm(100, 200, 1), gnm(100, 200, 1));
    EXPECT_NE(gnm(100, 200, 1), gnm(100, 200, 2));
}

TEST(Rmat, HasPowerLawLikeDegrees) {
    const input_graph g = rmat(12, 16, 1);
    EXPECT_EQ(g.n, 4096u);
    EXPECT_TRUE(is_canonical(g));
    EXPECT_LE(g.edges.size(), 16u * 4096u);  // repeats were removed

    std::vector<std::uint32_t> degree(g.n, 0);
    for (const edge e : g.edges) {
        ++degree[e.u];
        ++degree[e.v];
    }
    const double average = 2.0 * static_cast<double>(g.edges.size()) / g.n;
    const auto max_degree = *std::max_element(degree.begin(), degree.end());
    const auto isolated = std::count(degree.begin(), degree.end(), 0u);
    EXPECT_GT(max_degree, 10 * average) << "a few vertices have very high degree";
    EXPECT_GT(isolated, 0) << "and many have none";
}

TEST(Rmat, SeedSelectsTheGraph) {
    EXPECT_EQ(rmat(8, 4, 1), rmat(8, 4, 1));
    EXPECT_NE(rmat(8, 4, 1), rmat(8, 4, 2));
}

}  // namespace
}  // namespace elemgraph::gen
