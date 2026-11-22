// Q6 (DESIGN.md §1): what certifying 2-edge- and 2-vertex-connectivity costs
// compared with the plain test. Every part starts from the same dfs() result
// on the same csr_graph, as in Q4:
//
//   dfs             the search
//   plain           chain_connectivity() without keeping the chains: the
//                   test without a certificate
//   certify_edge    certify_two_edge_connectivity(): the same work, but the
//                   chains are kept, and turned into a certificate
//   certify_vertex  certify_two_vertex_connectivity()
//   check_edge      check_two_edge_connectivity() on that certificate
//   check_vertex    check_two_vertex_connectivity()
//
// grid and gnm_cycle are 2-vertex-connected, so both answers are yes and the
// certificates are ear decompositions with all m edges. gnm has bridges, so
// both answers are no, and the certificates are vertex sets.
//
// Names are certifying/<part>/<graph class>/<log2 n>, e.g.
// certifying/check_vertex/gnm_cycle/20.
#include <benchmark/benchmark.h>

#include <span>
#include <string>

#include "elemgraph/algorithms/certifying.hpp"
#include "elemgraph/algorithms/connectivity.hpp"
#include "elemgraph/algorithms/dfs.hpp"
#include "elemgraph/csr_graph.hpp"
#include "inputs.hpp"
#include "suites.hpp"

namespace elemgraph::bench {
namespace {

enum class part { dfs, plain, certify_edge, certify_vertex, check_edge, check_vertex };

std::string part_name(part p) {
    switch (p) {
    case part::dfs:
        return "dfs";
    case part::plain:
        return "plain";
    case part::certify_edge:
        return "certify_edge";
    case part::certify_vertex:
        return "certify_vertex";
    case part::check_edge:
        return "check_edge";
    case part::check_vertex:
        return "check_vertex";
    }
    return "?";
}

template <part timed>
void bm_certifying(benchmark::State& state, graph_class c, unsigned log_n) {
    // Generating and building the graph, the search for the later parts, and
    // the certificates that the checkers check are not timed (DESIGN.md §6).
    const csr_graph g(input(c, log_n));
    dfs_result search;
    dfs_workspace dfs_memory;
    dfs(g, search, dfs_memory);
    connectivity_result result;
    chain_workspace chain_memory;
    certifying_workspace certifying_memory;
    two_edge_connectivity_certificate edge;
    two_vertex_connectivity_certificate vertex;
    certify_two_edge_connectivity(g, search, edge, certifying_memory);
    certify_two_vertex_connectivity(g, search, vertex, certifying_memory);
    certificate_check_workspace check_memory;
    bool accepted = true;

    const auto run = [&] {
        if constexpr (timed == part::dfs) {
            dfs(g, search, dfs_memory);
            benchmark::DoNotOptimize(search.order.data());
        } else if constexpr (timed == part::plain) {
            chain_connectivity(g, search, result, chain_memory);
            benchmark::DoNotOptimize(result.bridges.data());
        } else if constexpr (timed == part::certify_edge) {
            certify_two_edge_connectivity(g, search, edge, certifying_memory);
            benchmark::DoNotOptimize(&edge);
        } else if constexpr (timed == part::certify_vertex) {
            certify_two_vertex_connectivity(g, search, vertex, certifying_memory);
            benchmark::DoNotOptimize(&vertex);
        } else if constexpr (timed == part::check_edge) {
            accepted = check_two_edge_connectivity(g, edge, check_memory);
            benchmark::DoNotOptimize(accepted);
        } else {
            accepted = check_two_vertex_connectivity(g, vertex, check_memory);
            benchmark::DoNotOptimize(accepted);
        }
        benchmark::ClobberMemory();
    };
    // One untimed call first, so that every timed call finds its memory
    // allocated, as in a program that reuses its workspaces.
    run();
    for (auto _ : state) {
        run();
    }
    if (!accepted) {
        state.SkipWithError("the checker rejected the certificate");
    }

    // The same counts for every part, so that each row of the results says
    // what the graph and its certificates look like.
    state.counters["n"] = static_cast<double>(g.num_vertices());
    state.counters["m"] = static_cast<double>(g.num_edges());
    state.counters["two_edge_connected"] = edge.two_edge_connected ? 1 : 0;
    state.counters["two_vertex_connected"] = vertex.two_vertex_connected ? 1 : 0;
    state.counters["ears"] = static_cast<double>(edge.ears.size());
    state.counters["side"] = static_cast<double>(vertex.side.size());
    state.counters["edges/s"] = benchmark::Counter(static_cast<double>(g.num_edges()),
                                                   benchmark::Counter::kIsIterationInvariantRate);
}

template <part timed>
void add(graph_class c, unsigned log_n) {
    const std::string graph = std::string(name(c)) + "/" + std::to_string(log_n);
    benchmark::RegisterBenchmark("certifying/" + part_name(timed) + "/" + graph,
                                 [c, log_n](benchmark::State& state) {
                                     bm_certifying<timed>(state, c, log_n);
                                 })
        ->UseRealTime()
        ->Unit(benchmark::kMicrosecond);
}

}  // namespace

void register_certifying(std::span<const unsigned> log_sizes) {
    for (const graph_class c : {graph_class::grid, graph_class::gnm_cycle, graph_class::gnm}) {
        for (const unsigned log_n : log_sizes) {
            add<part::dfs>(c, log_n);
            add<part::plain>(c, log_n);
            add<part::certify_edge>(c, log_n);
            add<part::certify_vertex>(c, log_n);
            add<part::check_edge>(c, log_n);
            add<part::check_vertex>(c, log_n);
        }
    }
}

}  // namespace elemgraph::bench
