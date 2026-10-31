// Converts test graphs into the library's input type.
#pragma once

#include "elemgraph/edge_list.hpp"
#include "test_graph.hpp"

namespace elemgraph::test {

// The edges keep their order, so an edge has the same id in g and in the
// result, and library results can be compared with the references directly.
inline edge_list to_edge_list(const input_graph& g) {
    edge_list list(g.n);
    list.reserve(g.edges.size());
    for (const auto [u, v] : g.edges) {
        list.add_edge(u, v);
    }
    return list;
}

}  // namespace elemgraph::test
