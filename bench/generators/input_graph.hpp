// The plain graph type shared by tests and benchmarks: vertices 0 .. n-1 and
// a list of undirected edges, where the id of an edge is its index in the
// list. Self-loops and parallel edges are allowed.
//
// Generators, readers and the brute-force references in tests/support/ all
// work on this type and share no code with the library.
#pragma once

#include <compare>
#include <cstdint>
#include <ostream>
#include <vector>

namespace elemgraph::gen {

using vertex = std::uint32_t;
using edge_id = std::uint32_t;

struct edge {
    vertex u;
    vertex v;
    auto operator<=>(const edge&) const = default;
};

struct input_graph {
    vertex n = 0;
    std::vector<edge> edges;
    bool operator==(const input_graph&) const = default;
};

// Let GoogleTest print edges and graphs in failure messages,
// e.g. "n=3 m=2 {0-1 1-2}".
inline std::ostream& operator<<(std::ostream& os, edge e) {
    return os << e.u << '-' << e.v;
}

inline std::ostream& operator<<(std::ostream& os, const input_graph& g) {
    os << "n=" << g.n << " m=" << g.edges.size() << " {";
    for (std::size_t i = 0; i < g.edges.size(); ++i) {
        os << (i == 0 ? "" : " ") << g.edges[i];
    }
    return os << '}';
}

}  // namespace elemgraph::gen
