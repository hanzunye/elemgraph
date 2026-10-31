// Compressed sparse row (CSR) graph, the main representation (DESIGN.md §4.2).
//
// The incidences of vertex v are stored at positions offsets_[v] up to
// offsets_[v + 1] - 1 of two parallel arrays: targets_ holds the other
// endpoint and edge_ids_ the id of the edge. Every edge {u, v} is stored twice,
// once at u and once at v, under the same id: its id in the edge_list the graph
// was built from. Parallel edges are kept. Self-loops are dropped, so the ids
// of a graph built from a list with self-loops have gaps. The incidences of
// each vertex are ordered by edge id.
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

#include "edge_list.hpp"

namespace elemgraph {

class csr_graph {
public:
    // Index into targets_ and edge_ids_. Fixed to 32 bits for now, like vertex
    // and edge_id; it becomes the template parameter E later (DESIGN.md §4.2).
    using offset = std::uint32_t;

    // Builds the graph from an edge list with a counting sort in O(n + m).
    explicit csr_graph(const edge_list& list) : n_(list.num_vertices()) {
        // Both arrays get two entries per edge, and their length must fit in
        // an offset. Checked first: the counts below would overflow silently.
        assert(list.num_edges() <= std::numeric_limits<offset>::max() / 2);
        const std::span<const edge> edges = list.edges();

        // 1. Count the edges at each vertex, one position to the right:
        //    offsets_[x + 1] becomes the degree of x.
        offsets_.assign(std::size_t{n_} + 1, 0);
        for (const auto [u, v] : edges) {
            if (u != v) {
                ++offsets_[u + 1];
                ++offsets_[v + 1];
            }
        }

        // 2. Prefix sums: offsets_[x] becomes the first position of x, and
        //    offsets_[n] the length of both arrays.
        std::inclusive_scan(offsets_.begin(), offsets_.end(), offsets_.begin());

        // 3. Put every edge at both ends. next[x] is the next free position of
        //    x. Going through the edges in id order keeps the edges of each
        //    vertex ordered by id.
        targets_.resize(offsets_[n_]);
        edge_ids_.resize(offsets_[n_]);
        std::vector<offset> next(offsets_.begin(), offsets_.end() - 1);
        for (edge_id id = 0; id < edges.size(); ++id) {
            const auto [u, v] = edges[id];
            if (u == v) {
                continue;
            }
            targets_[next[u]] = v;
            edge_ids_[next[u]] = id;
            ++next[u];
            targets_[next[v]] = u;
            edge_ids_[next[v]] = id;
            ++next[v];
        }
    }

    [[nodiscard]] vertex num_vertices() const { return n_; }

    // The number of stored edges: the edges of the list without its self-loops.
    [[nodiscard]] std::size_t num_edges() const {
        return targets_.size() / 2;
    }

    // The number of edges at v.
    [[nodiscard]] std::size_t degree(vertex v) const {
        assert(v < n_);
        return offsets_[v + 1] - offsets_[v];
    }

    // The other endpoint of each edge at v, ordered by edge id.
    [[nodiscard]] std::span<const vertex> neighbors(vertex v) const {
        assert(v < n_);
        return std::span<const vertex>(targets_).subspan(offsets_[v], degree(v));
    }

    // The id of each edge at v, in the same order as neighbors(v):
    // edge_ids(v)[i] is the id of the edge to neighbors(v)[i].
    [[nodiscard]] std::span<const edge_id> edge_ids(vertex v) const {
        assert(v < n_);
        return std::span<const edge_id>(edge_ids_).subspan(offsets_[v], degree(v));
    }

private:
    vertex n_;
    std::vector<offset> offsets_;    // n + 1 entries
    std::vector<vertex> targets_;    // 2 * num_edges() entries
    std::vector<edge_id> edge_ids_;  // parallel to targets_
};

}  // namespace elemgraph
