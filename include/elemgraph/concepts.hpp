// What algorithms require of a graph type (DESIGN.md §4.1). Each algorithm
// states the weakest concept it needs; tests/concepts_test.cpp checks which
// concepts csr_graph and adjacency_list satisfy.
#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>

namespace elemgraph {

// A graph that is built once and then only read: vertices 0 .. n-1 of an
// unsigned type, and for every vertex the other endpoints of its edges.
// neighbors(v) is a random-access range, so that a search can stop in the
// middle of a vertex's neighbors and continue later at the same index.
template <typename G>
concept StaticGraph = requires(const G& g, typename G::vertex_type v) {
    requires std::unsigned_integral<typename G::vertex_type>;
    { g.num_vertices() } -> std::convertible_to<std::size_t>;
    { g.num_edges() } -> std::convertible_to<std::size_t>;
    { g.degree(v) } -> std::convertible_to<std::size_t>;
    { g.neighbors(v) } -> std::ranges::random_access_range;
    requires std::convertible_to<std::ranges::range_value_t<decltype(g.neighbors(v))>,
                                 typename G::vertex_type>;
};

// A static graph that also knows the id of each edge at a vertex:
// edge_ids(v)[i] is the id of the edge to neighbors(v)[i]. Needed to tell
// parallel edges apart, e.g. for bridges.
template <typename G>
concept EdgeIndexedGraph = StaticGraph<G> && requires(const G& g, typename G::vertex_type v) {
    { g.edge_ids(v) } -> std::ranges::random_access_range;
};

}  // namespace elemgraph
