// Checks the certifying tests on random graphs: their answers against the
// brute-force references, their certificates against the checkers, and the
// checkers against certificates that were made wrong on purpose.
#include "elemgraph/algorithms/certifying.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "elemgraph/csr_graph.hpp"
#include "generators/synthetic.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"
#include "support/reference.hpp"

namespace elemgraph::test {
namespace {

bool is_connected(const input_graph& g) {
    const auto labels = connected_components(g);
    return std::all_of(labels.begin(), labels.end(), [](std::uint32_t l) { return l == 0; });
}

void expect_correct_and_certified(const input_graph& g) {
    const csr_graph csr(to_edge_list(g));
    const bool connected = is_connected(g);

    const auto edge = certify_two_edge_connectivity(csr);
    EXPECT_EQ(edge.two_edge_connected, g.n >= 2 && connected && bridges(g).empty());
    EXPECT_TRUE(check_two_edge_connectivity(csr, edge));

    const auto two_vertex = certify_two_vertex_connectivity(csr);
    EXPECT_EQ(two_vertex.two_vertex_connected,
              g.n >= 3 && connected && cut_vertices(g).empty());
    EXPECT_TRUE(check_two_vertex_connectivity(csr, two_vertex));
}

input_graph ear_graph(rng& r) {
    return random_ear_graph(r, static_cast<std::uint32_t>(r.below(15)), r.chance(1, 2));
}

TEST(CertifyingRandom, CorrectAndCertifiedOnMultigraphs) {
    for_each_random_graph(1, 1000, small_random_multigraph, expect_correct_and_certified);
}

TEST(CertifyingRandom, CorrectAndCertifiedOnSimpleGraphs) {
    for_each_random_graph(1001, 1000, small_random_simple_graph, expect_correct_and_certified);
}

TEST(CertifyingRandom, CorrectAndCertifiedOnTrees) {
    const auto make = [](rng& r) { return random_tree(r, static_cast<vertex>(r.below(40) + 1)); };
    for_each_random_graph(2001, 200, make, expect_correct_and_certified);
}

// Most of these are 2-edge-connected, and many 2-vertex-connected, so the
// ear decompositions are tested here.
TEST(CertifyingRandom, CorrectAndCertifiedOnGraphsMadeOfEars) {
    for_each_random_graph(3001, 1000, ear_graph, expect_correct_and_certified);
}

// The checkers are linear, so they also check the answers on graphs far too
// large for the brute-force references.
TEST(CertifyingRandom, CertifiedOnLargeGraphs) {
    rng r(4001);
    for (const input_graph& g :
         {gen::grid(200, 300), gen::gnm(50'000, 200'000, 1), gen::rmat(15, 8, 1),
          random_ear_graph(r, 20'000, true), random_ear_graph(r, 20'000, false)}) {
        const csr_graph csr(to_edge_list(g));
        EXPECT_TRUE(check_two_edge_connectivity(csr, certify_two_edge_connectivity(csr)));
        EXPECT_TRUE(check_two_vertex_connectivity(csr, certify_two_vertex_connectivity(csr)));
    }
    // A grid is 2-vertex-connected, and so is a graph of open ears.
    const csr_graph grid(to_edge_list(gen::grid(200, 300)));
    EXPECT_TRUE(certify_two_vertex_connectivity(grid).two_vertex_connected);
}

// --- Certificates made wrong on purpose. Each change below makes a correct
// certificate wrong, whatever the graph, so the checker must reject it.

// The ears as plain vectors, to change them and build them again.
struct ear_lists {
    std::vector<std::vector<vertex>> vertices;
    std::vector<std::vector<edge_id>> edges;
};

ear_lists lists_of(const chain_decomposition& ears) {
    ear_lists lists;
    for (std::size_t i = 0; i < ears.size(); ++i) {
        const auto vs = ears.vertices(i);
        const auto es = ears.edges(i);
        lists.vertices.emplace_back(vs.begin(), vs.end());
        lists.edges.emplace_back(es.begin(), es.end());
    }
    return lists;
}

chain_decomposition ears_of(const ear_lists& lists) {
    chain_decomposition ears;
    for (std::size_t i = 0; i < lists.vertices.size(); ++i) {
        ears.start(lists.vertices[i][0]);
        for (std::size_t j = 0; j < lists.edges[i].size(); ++j) {
            ears.extend(lists.edges[i][j], lists.vertices[i][j + 1]);
        }
        ears.finish();
    }
    return ears;
}

// Wrong versions of a correct ear decomposition of a graph with n vertices.
std::vector<chain_decomposition> wrong_ears(const chain_decomposition& correct, vertex n, rng& r) {
    const ear_lists lists = lists_of(correct);
    std::vector<chain_decomposition> wrong;

    // Without the last ear, its edges are on no ear.
    ear_lists dropped = lists;
    dropped.vertices.pop_back();
    dropped.edges.pop_back();
    wrong.push_back(ears_of(dropped));

    // A vertex replaced by another: the edges next to it no longer join the
    // vertices they are listed between (the graph has no self-loops).
    ear_lists moved = lists;
    const std::size_t i = r.below(moved.vertices.size());
    const std::size_t j = r.below(moved.vertices[i].size());
    moved.vertices[i][j] = static_cast<vertex>((moved.vertices[i][j] + 1 + r.below(n - 1)) % n);
    wrong.push_back(ears_of(moved));

    // An edge replaced by another edge of the decomposition, which is then
    // on two ears.
    std::vector<edge_id> all;
    for (const auto& es : lists.edges) {
        all.insert(all.end(), es.begin(), es.end());
    }
    ear_lists repeated = lists;
    const std::size_t k = r.below(repeated.edges.size());
    const std::size_t l = r.below(repeated.edges[k].size());
    edge_id other = all[r.below(all.size())];
    while (other == repeated.edges[k][l]) {
        other = all[r.below(all.size())];
    }
    repeated.edges[k][l] = other;
    wrong.push_back(ears_of(repeated));

    return wrong;
}

void expect_wrong_certificates_rejected(const input_graph& g) {
    const csr_graph csr(to_edge_list(g));
    rng r(g.edges.size());

    const auto edge = certify_two_edge_connectivity(csr);
    auto flipped_edge = edge;  // the other answer, without a certificate for it
    flipped_edge.two_edge_connected = !edge.two_edge_connected;
    EXPECT_FALSE(check_two_edge_connectivity(csr, flipped_edge));
    if (edge.two_edge_connected) {
        for (const chain_decomposition& ears : wrong_ears(edge.ears, g.n, r)) {
            auto wrong = edge;
            wrong.ears = ears;
            EXPECT_FALSE(check_two_edge_connectivity(csr, wrong));
        }
    } else if (g.n >= 2) {
        auto wrong = edge;
        wrong.side.push_back(edge.side.front());  // a vertex twice
        EXPECT_FALSE(check_two_edge_connectivity(csr, wrong));
        wrong.side.resize(g.n);  // all vertices
        for (vertex v = 0; v < g.n; ++v) {
            wrong.side[v] = v;
        }
        EXPECT_FALSE(check_two_edge_connectivity(csr, wrong));
    }

    const auto two_vertex = certify_two_vertex_connectivity(csr);
    auto flipped_vertex = two_vertex;
    flipped_vertex.two_vertex_connected = !two_vertex.two_vertex_connected;
    EXPECT_FALSE(check_two_vertex_connectivity(csr, flipped_vertex));
    if (two_vertex.two_vertex_connected) {
        for (const chain_decomposition& ears : wrong_ears(two_vertex.ears, g.n, r)) {
            auto wrong = two_vertex;
            wrong.ears = ears;
            EXPECT_FALSE(check_two_vertex_connectivity(csr, wrong));
        }
    } else if (g.n >= 3 && two_vertex.separator != no_vertex) {
        // The side has an edge to the separator, which then joins the rest.
        auto without_separator = two_vertex;
        without_separator.separator = no_vertex;
        EXPECT_FALSE(check_two_vertex_connectivity(csr, without_separator));
        auto separator_in_side = two_vertex;
        separator_in_side.side.push_back(two_vertex.separator);
        EXPECT_FALSE(check_two_vertex_connectivity(csr, separator_in_side));
    }
}

TEST(CertificateCheckerRandom, RejectsWrongCertificatesOnMultigraphs) {
    for_each_random_graph(5001, 1000, small_random_multigraph, expect_wrong_certificates_rejected);
}

TEST(CertificateCheckerRandom, RejectsWrongCertificatesOnGraphsMadeOfEars) {
    for_each_random_graph(6001, 1000, ear_graph, expect_wrong_certificates_rejected);
}

}  // namespace
}  // namespace elemgraph::test
