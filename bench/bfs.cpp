// Q1 for BFS (DESIGN.md §1): the same search on csr_graph and on
// adjacency_list, for every graph class and size. Each is run with both
// queue versions, which also compares the flat-array queue with std::queue.
//
// Names are bfs/<queue>/<representation>/<graph class>/<log2 n>, e.g.
// bfs/flat/csr_graph/rmat/20, so --benchmark_filter can pick any slice.
#include <benchmark/benchmark.h>

#include <cstddef>
#include <span>
#include <string>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/algorithms/bfs.hpp"
#include "elemgraph/csr_graph.hpp"
#include "inputs.hpp"
#include "suites.hpp"

namespace elemgraph::bench {
namespace {

enum class queue_kind { flat, std_queue };

// The search starts at a vertex of maximum degree. In every class this
// vertex lies in the largest component; vertex 0 might be isolated in R-MAT.
template <typename Graph>
vertex max_degree_vertex(const Graph& g) {
    vertex best = 0;
    for (vertex v = 1; v < g.num_vertices(); ++v) {
        if (g.degree(v) > g.degree(best)) {
            best = v;
        }
    }
    return best;
}

template <typename Graph, queue_kind queue>
void bm_bfs(benchmark::State& state, graph_class c, unsigned log_n) {
    // Generating and building the graph is not timed (DESIGN.md §6).
    const Graph g(input(c, log_n));
    const vertex source = max_degree_vertex(g);
    bfs_result result;
    bfs_workspace workspace;

    for (auto _ : state) {
        if constexpr (queue == queue_kind::flat) {
            bfs(g, source, result, workspace);
        } else {
            bfs_std_queue(g, source, result);
        }
        benchmark::DoNotOptimize(result.distance.data());
        benchmark::ClobberMemory();
    }

    // Every edge of the searched component is scanned from both ends.
    // edges/s counts each edge once, like Graph500's TEPS.
    std::size_t reached = 0;
    std::size_t scanned = 0;
    for (vertex v = 0; v < g.num_vertices(); ++v) {
        if (result.distance[v] != unreachable) {
            ++reached;
            scanned += g.degree(v);
        }
    }
    state.counters["n"] = static_cast<double>(g.num_vertices());
    state.counters["m"] = static_cast<double>(g.num_edges());
    state.counters["reached"] = static_cast<double>(reached);
    state.counters["edges/s"] = benchmark::Counter(static_cast<double>(scanned) / 2,
                                                   benchmark::Counter::kIsIterationInvariantRate);
}

template <typename Graph, queue_kind queue>
void add(const std::string& name, graph_class c, unsigned log_n) {
    benchmark::RegisterBenchmark(name, [c, log_n](benchmark::State& state) {
        bm_bfs<Graph, queue>(state, c, log_n);
    })->UseRealTime()->Unit(benchmark::kMicrosecond);
}

}  // namespace

void register_bfs(std::span<const unsigned> log_sizes) {
    for (const graph_class c : all_graph_classes) {
        for (const unsigned log_n : log_sizes) {
            const std::string graph = std::string(name(c)) + "/" + std::to_string(log_n);
            add<csr_graph, queue_kind::flat>("bfs/flat/csr_graph/" + graph, c, log_n);
            add<adjacency_list, queue_kind::flat>("bfs/flat/adjacency_list/" + graph, c, log_n);
            add<csr_graph, queue_kind::std_queue>("bfs/std_queue/csr_graph/" + graph, c, log_n);
            add<adjacency_list, queue_kind::std_queue>("bfs/std_queue/adjacency_list/" + graph, c,
                                                       log_n);
        }
    }
}

}  // namespace elemgraph::bench
