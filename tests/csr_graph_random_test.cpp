// Checks csr_graph against a direct construction on random multigraphs.
#include "elemgraph/csr_graph.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"

namespace elemgraph::test {
namespace {

// incidences[v] lists (other endpoint, edge id) for every edge at v.
using incidences = std::vector<std::vector<std::pair<vertex, edge_id>>>;

// What a csr_graph built from g has to store: every edge {u, v} with u != v
// at both ends under its id, and the edges of each vertex in id order.
incidences expected_incidences(const input_graph& g) {
    incidences inc(g.n);
    for (edge_id e = 0; e < g.edges.size(); ++e) {
        const auto [u, v] = g.edges[e];
        if (u != v) {
            inc[u].push_back({v, e});
            inc[v].push_back({u, e});
        }
    }
    return inc;
}

// What g stores, read through its public interface.
incidences stored_incidences(const csr_graph& g) {
    incidences inc(g.num_vertices());
    for (vertex v = 0; v < g.num_vertices(); ++v) {
        const auto targets = g.neighbors(v);
        const auto ids = g.edge_ids(v);
        EXPECT_EQ(g.degree(v), targets.size()) << "vertex " << v;
        EXPECT_EQ(ids.size(), targets.size()) << "vertex " << v;
        for (std::size_t i = 0; i < std::min(targets.size(), ids.size()); ++i) {
            inc[v].push_back({targets[i], ids[i]});
        }
    }
    return inc;
}

TEST(CsrGraphRandom, StoresExactlyTheEdgesOfTheList) {
    for_each_random_graph(1, 1000, small_random_multigraph, [](const input_graph& g) {
        const csr_graph csr(to_edge_list(g));
        const auto loops = std::count_if(g.edges.begin(), g.edges.end(),
                                         [](edge e) { return e.u == e.v; });
        EXPECT_EQ(csr.num_vertices(), g.n);
        EXPECT_EQ(csr.num_edges(), g.edges.size() - static_cast<std::size_t>(loops));
        EXPECT_EQ(stored_incidences(csr), expected_incidences(g));
    });
}

}  // namespace
}  // namespace elemgraph::test
