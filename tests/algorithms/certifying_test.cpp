// Hand-written cases for the certifying tests and their checkers. The DFS
// orders in the comments follow from vertex 0 as root and neighbors ordered
// by edge id.
#include "elemgraph/algorithms/certifying.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <utility>
#include <vector>

#include "elemgraph/csr_graph.hpp"

namespace elemgraph {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

csr_graph graph(vertex n, const std::vector<std::pair<vertex, vertex>>& edges) {
    edge_list list(n);
    for (const auto& [u, v] : edges) {
        list.add_edge(u, v);
    }
    return csr_graph(list);
}

// Appends an ear with the given vertices and the edges between them.
void add_ear(chain_decomposition& ears, const std::vector<vertex>& vertices,
             const std::vector<edge_id>& edges) {
    ASSERT_EQ(vertices.size(), edges.size() + 1);
    ears.start(vertices[0]);
    for (std::size_t j = 0; j < edges.size(); ++j) {
        ears.extend(edges[j], vertices[j + 1]);
    }
    ears.finish();
}

two_edge_connectivity_certificate two_edge_yes(const std::vector<std::vector<vertex>>& vertices,
                                               const std::vector<std::vector<edge_id>>& edges) {
    two_edge_connectivity_certificate c;
    c.two_edge_connected = true;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        add_ear(c.ears, vertices[i], edges[i]);
    }
    return c;
}

two_vertex_connectivity_certificate two_vertex_yes(
    const std::vector<std::vector<vertex>>& vertices,
    const std::vector<std::vector<edge_id>>& edges) {
    two_vertex_connectivity_certificate c;
    c.two_vertex_connected = true;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        add_ear(c.ears, vertices[i], edges[i]);
    }
    return c;
}

two_edge_connectivity_certificate two_edge_no(std::vector<vertex> side) {
    two_edge_connectivity_certificate c;
    c.side = std::move(side);
    return c;
}

two_vertex_connectivity_certificate two_vertex_no(vertex separator, std::vector<vertex> side) {
    two_vertex_connectivity_certificate c;
    c.separator = separator;
    c.side = std::move(side);
    return c;
}

csr_graph triangle() { return graph(3, {{0, 1}, {1, 2}, {2, 0}}); }

// --- The tests ---

TEST(Certifying, TooFewVerticesNeedNoCertificate) {
    for (const vertex n : {0u, 1u}) {
        const csr_graph g = graph(n, {});
        const auto edge = certify_two_edge_connectivity(g);
        EXPECT_FALSE(edge.two_edge_connected);
        EXPECT_THAT(edge.side, IsEmpty());
        EXPECT_TRUE(check_two_edge_connectivity(g, edge));
    }
    for (const vertex n : {0u, 1u, 2u}) {
        const csr_graph g = graph(n, {});
        const auto two_vertex = certify_two_vertex_connectivity(g);
        EXPECT_FALSE(two_vertex.two_vertex_connected);
        EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
    }
}

TEST(Certifying, TwoParallelEdgesAreTwoEdgeConnectedOnly) {
    // DFS 0 -> 1 through edge 0; edge 1 is the back edge (0, 1).
    const csr_graph g = graph(2, {{0, 1}, {0, 1}});
    const auto edge = certify_two_edge_connectivity(g);
    ASSERT_TRUE(edge.two_edge_connected);
    ASSERT_EQ(edge.ears.size(), 1u);
    EXPECT_THAT(edge.ears.vertices(0), ElementsAre(0u, 1u, 0u));
    EXPECT_THAT(edge.ears.edges(0), ElementsAre(1u, 0u));
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    // An open ear decomposition, but two vertices are too few.
    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_FALSE(two_vertex.two_vertex_connected);
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_yes({{0, 1, 0}}, {{1, 0}})));
}

TEST(Certifying, TriangleIsBoth) {
    const csr_graph g = triangle();
    const auto edge = certify_two_edge_connectivity(g);
    ASSERT_TRUE(edge.two_edge_connected);
    ASSERT_EQ(edge.ears.size(), 1u);
    EXPECT_THAT(edge.ears.vertices(0), ElementsAre(0u, 2u, 1u, 0u));
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    const auto two_vertex = certify_two_vertex_connectivity(g);
    ASSERT_TRUE(two_vertex.two_vertex_connected);
    EXPECT_THAT(two_vertex.ears.vertices(0), ElementsAre(0u, 2u, 1u, 0u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, CycleWithChordHasAnOpenEar) {
    // The chains are the cycle 0, 3, 2, 1, 0 and the chord 1-3 as a path.
    const csr_graph g = graph(4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {1, 3}});
    const auto two_vertex = certify_two_vertex_connectivity(g);
    ASSERT_TRUE(two_vertex.two_vertex_connected);
    ASSERT_EQ(two_vertex.ears.size(), 2u);
    EXPECT_THAT(two_vertex.ears.vertices(1), ElementsAre(1u, 3u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, PathIsSplitAtItsFirstBridge) {
    // DFS 0 -> 1 -> 2. The first bridge in preorder is t(1) = {0, 1}.
    const csr_graph g = graph(3, {{0, 1}, {1, 2}});
    const auto edge = certify_two_edge_connectivity(g);
    EXPECT_FALSE(edge.two_edge_connected);
    EXPECT_THAT(edge.side, ElementsAre(1u, 2u));  // the subtree of 1
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    // 1 has a second edge, so 1 separates its descendants from the rest.
    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_FALSE(two_vertex.two_vertex_connected);
    EXPECT_EQ(two_vertex.separator, 1u);
    EXPECT_THAT(two_vertex.side, ElementsAre(2u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, LeafIsSeparatedByItsParent) {
    // DFS 0 -> 1, then 0 -> 2. The first bridge is t(1), and 1 is a leaf.
    const csr_graph g = graph(3, {{0, 1}, {0, 2}});
    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_FALSE(two_vertex.two_vertex_connected);
    EXPECT_EQ(two_vertex.separator, 0u);
    EXPECT_THAT(two_vertex.side, ElementsAre(1u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, BridgeNextToParallelEdges) {
    // 0 - 1 = 2: t(1) = {0, 1} is a bridge; 1 and 2 are joined twice.
    const csr_graph g = graph(3, {{0, 1}, {1, 2}, {2, 1}});
    const auto edge = certify_two_edge_connectivity(g);
    EXPECT_FALSE(edge.two_edge_connected);
    EXPECT_THAT(edge.side, ElementsAre(1u, 2u));
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_EQ(two_vertex.separator, 1u);
    EXPECT_THAT(two_vertex.side, ElementsAre(2u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, TwoTrianglesSharingAVertex) {
    // DFS 0 -> 1 -> 2 -> 3 -> 4. Chains: 0, 2, 1, 0 and 2, 4, 3, 2. The
    // second is a cycle at 2 that ends with t(3), so 2 separates {3, 4}.
    const csr_graph g = graph(5, {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}});
    const auto edge = certify_two_edge_connectivity(g);
    ASSERT_TRUE(edge.two_edge_connected);
    ASSERT_EQ(edge.ears.size(), 2u);
    EXPECT_TRUE(edge.ears.is_cycle(1));  // a closed ear
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_FALSE(two_vertex.two_vertex_connected);
    EXPECT_EQ(two_vertex.separator, 2u);
    EXPECT_THAT(two_vertex.side, ElementsAre(3u, 4u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));

    // The same ears do not prove 2-vertex-connectivity: the second is closed.
    two_vertex_connectivity_certificate wrong;
    wrong.two_vertex_connected = true;
    wrong.ears = edge.ears;
    EXPECT_FALSE(check_two_vertex_connectivity(g, wrong));
}

TEST(Certifying, DisconnectedGraphIsSplitIntoItsFirstComponent) {
    const csr_graph g = graph(6, {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}});
    const auto edge = certify_two_edge_connectivity(g);
    EXPECT_FALSE(edge.two_edge_connected);
    EXPECT_THAT(edge.side, ElementsAre(0u, 1u, 2u));
    EXPECT_TRUE(check_two_edge_connectivity(g, edge));

    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_FALSE(two_vertex.two_vertex_connected);
    EXPECT_EQ(two_vertex.separator, no_vertex);
    EXPECT_THAT(two_vertex.side, ElementsAre(0u, 1u, 2u));
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

TEST(Certifying, SelfLoopsAreIgnored) {
    const csr_graph g = graph(3, {{0, 1}, {1, 1}, {1, 2}, {2, 0}, {0, 0}});
    EXPECT_TRUE(certify_two_edge_connectivity(g).two_edge_connected);
    const auto two_vertex = certify_two_vertex_connectivity(g);
    EXPECT_TRUE(two_vertex.two_vertex_connected);
    EXPECT_TRUE(check_two_vertex_connectivity(g, two_vertex));
}

// --- The checkers on certificates written by hand ---

TEST(CertificateChecker, AcceptsAnEarInEitherDirection) {
    // Edges: 0 = {0, 1}, 1 = {1, 2}, 2 = {2, 0}.
    EXPECT_TRUE(check_two_edge_connectivity(triangle(), two_edge_yes({{0, 1, 2, 0}}, {{0, 1, 2}})));
    EXPECT_TRUE(check_two_edge_connectivity(triangle(), two_edge_yes({{1, 0, 2, 1}}, {{0, 2, 1}})));
}

TEST(CertificateChecker, RejectsWrongEarDecompositions) {
    const csr_graph g = triangle();
    // No ears.
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_yes({}, {})));
    // The first ear is not a cycle.
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_yes({{0, 1, 2}, {2, 0}}, {{0, 1}, {2}})));
    // An edge listed between vertices it does not join.
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_yes({{0, 1, 2, 0}}, {{0, 2, 1}})));
    // An edge on two ears.
    EXPECT_FALSE(
        check_two_edge_connectivity(g, two_edge_yes({{0, 1, 2, 0}, {0, 2}}, {{0, 1, 2}, {2}})));
    // An edge on no ear.
    const csr_graph chord = graph(4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {1, 3}});
    EXPECT_FALSE(
        check_two_edge_connectivity(chord, two_edge_yes({{0, 1, 2, 3, 0}}, {{0, 1, 2, 3}})));
    // A vertex on no ear: the triangle plus an isolated vertex.
    const csr_graph isolated = graph(4, {{0, 1}, {1, 2}, {2, 0}});
    EXPECT_FALSE(check_two_edge_connectivity(isolated, two_edge_yes({{0, 1, 2, 0}}, {{0, 1, 2}})));
    // An ear whose inner vertex is already on an earlier ear.
    EXPECT_FALSE(check_two_edge_connectivity(
        chord, two_edge_yes({{0, 1, 2, 3, 0}, {1, 3, 1}}, {{0, 1, 2, 3}, {4, 4}})));
    // Vertices and edges that do not exist.
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_yes({{0, 1, 7, 0}}, {{0, 1, 2}})));
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_yes({{0, 1, 2, 0}}, {{0, 1, 9}})));
}

TEST(CertificateChecker, RejectsEarsWithoutEdges) {
    two_edge_connectivity_certificate c;
    c.two_edge_connected = true;
    c.ears.start(0);
    c.ears.finish();
    EXPECT_FALSE(check_two_edge_connectivity(triangle(), c));
}

TEST(CertificateChecker, RejectsWrongEdgeCuts) {
    const csr_graph g = triangle();
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_no({0})));        // two edges leave it
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_no({})));         // empty
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_no({0, 1, 2})));  // all vertices
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_no({0, 0})));     // repeated
    EXPECT_FALSE(check_two_edge_connectivity(g, two_edge_no({5})));        // no such vertex
}

TEST(CertificateChecker, RejectsWrongSeparations) {
    const csr_graph g = triangle();
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_no(0, {1})));  // edge 1-2
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_no(0, {1, 2})));  // nothing left
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_no(0, {0})));  // side holds the separator
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_no(7, {1})));  // no such vertex
    EXPECT_FALSE(check_two_vertex_connectivity(g, two_vertex_no(no_vertex, {})));
}

}  // namespace
}  // namespace elemgraph
