// A minimal undirected multigraph for tests: vertices 0 .. n-1 and a list of
// edges, where the id of an edge is its index in the list. Self-loops and
// parallel edges are allowed.
//
// The random generators and the brute-force references work on this type
// only, so they share no code with the library they are used to check.
#pragma once

#include <compare>
#include <cstdint>
#include <ostream>
#include <vector>

namespace elemgraph::test {

using vertex = std::uint32_t;
using edge_id = std::uint32_t;

struct edge {
    vertex u;
    vertex v;
    auto operator<=>(const edge&) const = default;
};

struct test_graph {
    vertex n = 0;
    std::vector<edge> edges;
    bool operator==(const test_graph&) const = default;
};

// Let GoogleTest print edges and graphs in failure messages,
// e.g. "n=3 m=2 {0-1 1-2}".
inline std::ostream& operator<<(std::ostream& os, edge e) {
    return os << e.u << '-' << e.v;
}

inline std::ostream& operator<<(std::ostream& os, const test_graph& g) {
    os << "n=" << g.n << " m=" << g.edges.size() << " {";
    for (std::size_t i = 0; i < g.edges.size(); ++i) {
        os << (i == 0 ? "" : " ") << g.edges[i];
    }
    return os << '}';
}

}  // namespace elemgraph::test
