// Random numbers that are the same on every platform.
//
// std::mt19937_64 is fully specified by the C++ standard, but
// std::uniform_int_distribution and std::shuffle are not: libstdc++ (Linux)
// and libc++ (macOS) turn the same engine output into different numbers. So
// bounded integers and shuffling are implemented here. This way a seed gives
// the same graph in tests and benchmarks on every machine.
#pragma once

#include <cassert>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace elemgraph::gen {

class rng {
public:
    explicit rng(std::uint64_t seed) : engine_(seed) {}

    // Uniform integer in [0, bound). Plain `x % bound` would favour small
    // results, so draws below 2^64 mod bound are rejected first.
    std::uint64_t below(std::uint64_t bound) {
        assert(bound > 0);
        const std::uint64_t threshold = -bound % bound;  // 2^64 mod bound
        std::uint64_t x = engine_();
        while (x < threshold) {
            x = engine_();
        }
        return x % bound;
    }

    // True with probability numerator / denominator.
    bool chance(std::uint64_t numerator, std::uint64_t denominator) {
        return below(denominator) < numerator;
    }

    // Fisher–Yates shuffle.
    template <typename T>
    void shuffle(std::vector<T>& items) {
        for (std::size_t i = items.size(); i > 1; --i) {
            std::swap(items[i - 1], items[below(i)]);
        }
    }

private:
    std::mt19937_64 engine_;
};

}  // namespace elemgraph::gen
