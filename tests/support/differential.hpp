// Runs a check on many seeded random graphs.
#pragma once

#include <gtest/gtest.h>

#include <cstdint>

#include "random_graphs.hpp"
#include "test_graph.hpp"

namespace elemgraph::test {

// Calls check(g) for `count` graphs g = make(r), where r = rng(seed) and seed
// runs from first_seed upwards. Failures inside check() are reported with the
// seed and the graph, and the loop stops after the first graph that fails, so
// that one bug does not produce thousands of messages. To debug a failure,
// rebuild its graph in a separate test:
//     rng r(seed);
//     input_graph g = small_random_multigraph(r);
template <typename Make, typename Check>
void for_each_random_graph(std::uint64_t first_seed, std::uint64_t count, Make make, Check check) {
    for (std::uint64_t seed = first_seed; seed < first_seed + count; ++seed) {
        rng r(seed);
        const input_graph g = make(r);
        SCOPED_TRACE(::testing::Message() << "seed " << seed << ": " << g);
        check(g);
        if (::testing::Test::HasFailure()) {
            return;
        }
    }
}

}  // namespace elemgraph::test
