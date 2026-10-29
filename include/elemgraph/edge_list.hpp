// A list of undirected edges over the vertices 0 .. n-1: the input from which
// the graph representations (csr_graph, adjacency_list) are built.
//
// The id of an edge is its position in the list. Self-loops and parallel edges
// are stored as given; each representation's builder decides what to keep.

#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <span>
#include <vector>

namespace elemgraph {

// v0.1 fixes both types to 32 bits; they become template parameters later
// (DESIGN.md §4.2).
using vertex = std::uint32_t;
using edge_id = std::uint32_t;

struct edge {
    vertex u;
    vertex v;
    // Generates ==, != and <, so edges can be compared in tests and sorted.
    auto operator<=>(const edge&) const = default;
};

// Prints an edge as "u-v", so that GoogleTest failure messages are readable.
inline std::ostream& operator<<(std::ostream& os, edge e) {
    os << e.u << '-' << e.v;
    return os;
}

class edge_list {
public:
    // A graph with n vertices and no edges.
    explicit edge_list(vertex n) : n_(n) {}

    // Makes room for m edges, so that adding them does not reallocate.
    void reserve(std::size_t m) {
        edges_.reserve(m);
    }

    // Adds the edge {u, v} and returns its id. Requires u < n and v < n.
    edge_id add_edge(vertex u, vertex v) {
        assert(u < n_ && v < n_);
        assert(edges_.size() < std::numeric_limits<edge_id>::max());
        const auto id = static_cast<edge_id>(edges_.size());
        edges_.push_back({u, v});
        return id;
    }

    [[nodiscard]] vertex num_vertices() const { return n_; }

    [[nodiscard]] std::size_t num_edges() const {
        return edges_.size();
    }

    // All edges, ordered by id. The view is invalidated by the next add_edge().
    [[nodiscard]] std::span<const edge> edges() const {
        return edges_;
    }

private:
    vertex n_;
    std::vector<edge> edges_;
};

}  // namespace elemgraph

