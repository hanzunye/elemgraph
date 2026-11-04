// Compile-time checks of which concepts the graph types satisfy. The file has
// no runtime tests: if a check fails, the test binary does not build.
#include "elemgraph/concepts.hpp"

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"
#include "elemgraph/edge_list.hpp"

namespace elemgraph {
namespace {

static_assert(StaticGraph<csr_graph>);
static_assert(EdgeIndexedGraph<csr_graph>);

static_assert(StaticGraph<adjacency_list>);
static_assert(!EdgeIndexedGraph<adjacency_list>);  // stores no edge ids

static_assert(!StaticGraph<edge_list>);  // no neighbors(v)
static_assert(!StaticGraph<int>);

}  // namespace
}  // namespace elemgraph
