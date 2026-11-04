// Adjacency list: one std::vector of neighbors per vertex. The baseline that
// csr_graph is compared with in Q1 (DESIGN.md §4.2): the same neighbors in the
// same order, but each vertex's list is a separate heap allocation instead of a
// slice of one contiguous array.
//
// It stores what csr_graph stores, apart from the edge ids: every edge {u, v}
// appears at u and at v, parallel edges are kept, self-loops are dropped, and
// the neighbors of each vertex are ordered by edge id.
#pragma once

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

#include "edge_list.hpp"

namespace elemgraph {

class adjacency_list {
public:
    using vertex_type = vertex;

    explicit adjacency_list(const edge_list& list) : adj_(list.num_vertices()) {
        // Each list gets its degree as capacity before it is filled, so that
        // Q1 compares the two layouts and not the spare capacity that
        // push_back leaves when a vector grows.
        std::vector<std::size_t> degrees(adj_.size(), 0);
        for (const auto [u, v] : list.edges()) {
            if (u != v) {
                ++degrees[u];
                ++degrees[v];
            }
        }
        for (std::size_t x = 0; x < adj_.size(); ++x) {
            adj_[x].reserve(degrees[x]);
        }

        // In id order, so that the neighbors of each vertex are ordered by id.
        for (const auto [u, v] : list.edges()) {
            if (u != v) {
                adj_[u].push_back(v);
                adj_[v].push_back(u);
                ++num_edges_;
            }
        }
    }

    [[nodiscard]] vertex num_vertices() const { return static_cast<vertex>(adj_.size()); }

    // The number of stored edges: the edges of the list without its self-loops.
    [[nodiscard]] std::size_t num_edges() const { return num_edges_; }

    // The number of edges at v.
    [[nodiscard]] std::size_t degree(vertex v) const {
        assert(v < num_vertices());
        return adj_[v].size();
    }

    // The other endpoint of each edge at v, ordered by edge id.
    [[nodiscard]] std::span<const vertex> neighbors(vertex v) const {
        assert(v < num_vertices());
        return adj_[v];
    }

private:
    std::vector<std::vector<vertex>> adj_;  // adj_[v]: the neighbors of v
    std::size_t num_edges_ = 0;
};

}  // namespace elemgraph
