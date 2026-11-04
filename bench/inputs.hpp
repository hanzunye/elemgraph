// The graphs that benchmarks run on (DESIGN.md §6): a graph class and a size
// give a canonical simple graph, generated with a fixed seed so that every
// machine benchmarks the same graphs.
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "elemgraph/edge_list.hpp"
#include "generators/input_graph.hpp"
#include "generators/synthetic.hpp"

namespace elemgraph::bench {

enum class graph_class { grid, gnm, rmat };

inline constexpr graph_class all_graph_classes[] = {graph_class::grid, graph_class::gnm,
                                                    graph_class::rmat};

inline std::string_view name(graph_class c) {
    switch (c) {
    case graph_class::grid:
        return "grid";
    case graph_class::gnm:
        return "gnm";
    case graph_class::rmat:
        return "rmat";
    }
    return "?";
}

inline constexpr std::uint64_t seed = 1;

// A graph of class c with 2^log_n vertices:
//   grid  2^(log_n / 2) rows, the rest as columns: average degree just under 4.
//   gnm   4 n edges: average degree 8.
//   rmat  edge factor 8, so 8 n edges are drawn and fewer remain after repeats
//         and self-loops are removed; many vertices stay isolated.
inline gen::input_graph generate(graph_class c, unsigned log_n) {
    const vertex n = vertex{1} << log_n;
    switch (c) {
    case graph_class::grid: {
        const vertex rows = vertex{1} << (log_n / 2);
        return gen::grid(rows, n / rows);
    }
    case graph_class::gnm:
        return gen::gnm(n, 4 * n, seed);
    case graph_class::rmat:
        return gen::rmat(log_n, 8, seed);
    }
    return {};
}

inline edge_list to_edge_list(const gen::input_graph& g) {
    edge_list list(g.n);
    list.reserve(g.edges.size());
    for (const auto [u, v] : g.edges) {
        list.add_edge(u, v);
    }
    return list;
}

// The edge list of graph class c with 2^log_n vertices. Only the graph asked
// for last is kept: the benchmarks of one graph are registered one after the
// other, so each graph is generated once, and at most one large graph is in
// memory.
inline const edge_list& input(graph_class c, unsigned log_n) {
    static std::optional<edge_list> cached;
    static graph_class cached_class{};
    static unsigned cached_log_n = 0;
    if (!cached || cached_class != c || cached_log_n != log_n) {
        cached.reset();  // free the old graph before generating the new one
        cached.emplace(to_edge_list(generate(c, log_n)));
        cached_class = c;
        cached_log_n = log_n;
    }
    return *cached;
}

}  // namespace elemgraph::bench
