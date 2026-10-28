// The plain graph type shared by tests and benchmarks: vertices 0 .. n-1 and
// a list of undirected edges, where the id of an edge is its index in the
// list. Self-loops and parallel edges are allowed.
//
// Generators, readers and the brute-force references in tests/support/ all
// work on this type and share no code with the library.
#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <ostream>
#include <utility>
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

// Turns g into a simple graph in canonical form: every edge is stored as
// (smaller, larger), self-loops and repeated edges are removed, and the edges
// are sorted. Benchmark inputs are always canonical (DESIGN.md §6). The DIMACS
// files store every road twice and R-MAT repeats many edges; read as they
// are, the repeats would be parallel edges and no road could be a bridge.
inline void canonicalize(input_graph& g) {
    std::erase_if(g.edges, [](edge e) { return e.u == e.v; });
    for (edge& e : g.edges) {
        if (e.u > e.v) {
            std::swap(e.u, e.v);
        }
    }
    std::sort(g.edges.begin(), g.edges.end());
    g.edges.erase(std::unique(g.edges.begin(), g.edges.end()), g.edges.end());
}

// True iff g is in the form produced by canonicalize(): every edge is
// (smaller, larger) with both ends below n, and the edges are strictly
// increasing (so there are no repeats).
inline bool is_canonical(const input_graph& g) {
    for (std::size_t i = 0; i < g.edges.size(); ++i) {
        const edge e = g.edges[i];
        if (e.u >= e.v || e.v >= g.n || (i > 0 && !(g.edges[i - 1] < e))) {
            return false;
        }
    }
    return true;
}

}  // namespace elemgraph::gen
