// Q4 (DESIGN.md §1): bridges and cut vertices with the lowpoint algorithm and
// with the chain decomposition. Both start from the same dfs() result on the
// same csr_graph and give the same output, so the search is timed once, on
// its own, and each algorithm is timed only for what it does after the
// search (DESIGN.md §4.4).
//
// Names are connectivity/<part>/<graph class>/<log2 n>, with part dfs,
// lowpoint or chains, e.g. connectivity/chains/rmat/20.
#include <benchmark/benchmark.h>

#include <span>
#include <string>

#include "elemgraph/algorithms/connectivity.hpp"
#include "elemgraph/algorithms/dfs.hpp"
#include "elemgraph/csr_graph.hpp"
#include "inputs.hpp"
#include "suites.hpp"

namespace elemgraph::bench {
namespace {

enum class part { dfs, lowpoint, chains };

std::string part_name(part p) {
    switch (p) {
    case part::dfs:
        return "dfs";
    case part::lowpoint:
        return "lowpoint";
    case part::chains:
        return "chains";
    }
    return "?";
}

template <part timed>
void bm_connectivity(benchmark::State& state, graph_class c, unsigned log_n) {
    // Generating and building the graph and the search for the lowpoint and
    // chain parts are not timed (DESIGN.md §6).
    const csr_graph g(input(c, log_n));
    dfs_result search;
    dfs_workspace dfs_memory;
    dfs(g, search, dfs_memory);
    connectivity_result result;
    lowpoint_workspace lowpoint_memory;
    chain_workspace chain_memory;

    const auto run = [&] {
        if constexpr (timed == part::dfs) {
            dfs(g, search, dfs_memory);
            benchmark::DoNotOptimize(search.order.data());
        } else if constexpr (timed == part::lowpoint) {
            lowpoint_connectivity(g, search, result, lowpoint_memory);
            benchmark::DoNotOptimize(result.bridges.data());
        } else {
            chain_connectivity(g, search, result, chain_memory);
            benchmark::DoNotOptimize(result.bridges.data());
        }
        benchmark::ClobberMemory();
    };
    // One untimed call first, so that every timed call finds its memory
    // allocated, as in a program that reuses its workspaces.
    run();
    for (auto _ : state) {
        run();
    }

    // The same counts for every part, so that each row of the results says
    // what the graph looks like.
    if constexpr (timed == part::dfs) {
        lowpoint_connectivity(g, search, result, lowpoint_memory);
    }
    state.counters["n"] = static_cast<double>(g.num_vertices());
    state.counters["m"] = static_cast<double>(g.num_edges());
    state.counters["bridges"] = static_cast<double>(result.bridges.size());
    state.counters["cut_vertices"] = static_cast<double>(result.cut_vertices.size());
    state.counters["edges/s"] = benchmark::Counter(static_cast<double>(g.num_edges()),
                                                   benchmark::Counter::kIsIterationInvariantRate);
}

template <part timed>
void add(graph_class c, unsigned log_n) {
    const std::string graph = std::string(name(c)) + "/" + std::to_string(log_n);
    benchmark::RegisterBenchmark("connectivity/" + part_name(timed) + "/" + graph,
                                 [c, log_n](benchmark::State& state) {
                                     bm_connectivity<timed>(state, c, log_n);
                                 })
        ->UseRealTime()
        ->Unit(benchmark::kMicrosecond);
}

}  // namespace

void register_connectivity(std::span<const unsigned> log_sizes) {
    for (const graph_class c : all_graph_classes) {
        for (const unsigned log_n : log_sizes) {
            add<part::dfs>(c, log_n);
            add<part::lowpoint>(c, log_n);
            add<part::chains>(c, log_n);
        }
    }
}

}  // namespace elemgraph::bench
