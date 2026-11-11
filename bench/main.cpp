// Benchmark driver. Results for the README come from bench/run.sh, which
// builds the release preset, checks and records the machine's settings, and
// runs this binary 10 times per benchmark into bench/results/<machine>/.
//
// --smoke runs every benchmark on tiny graphs only. CI does that, together
// with --benchmark_min_time=1x, to check that the benchmarks still run; CI
// timings are never reported (DESIGN.md §6).
#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

#include "suites.hpp"

#if defined(__APPLE__)
#include <pthread/qos.h>
#include <sys/sysctl.h>
#endif

namespace {

std::string cpu_model() {
#if defined(__APPLE__)
    std::array<char, 256> buffer{};
    std::size_t size = buffer.size();
    if (sysctlbyname("machdep.cpu.brand_string", buffer.data(), &size, nullptr, 0) == 0) {
        return buffer.data();
    }
#elif defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    for (std::string line; std::getline(cpuinfo, line);) {
        if (line.starts_with("model name")) {
            return line.substr(line.find(':') + 2);
        }
    }
#endif
    return "unknown";
}

#if defined(__APPLE__)
// Google Benchmark reports the caches of one kind of core only; on Apple
// silicon these are the efficiency cores. Records both kinds: perflevel0 are
// the performance cores, perflevel1 the efficiency cores. Defined on macOS
// only: elsewhere it would be unused, which -Werror turns into an error.
std::string apple_core_caches() {
    const auto number = [](const char* key) -> std::string {
        // Some keys are 4 bytes, others 8. value starts at 0, so on a
        // little-endian CPU a 4-byte answer still reads correctly.
        std::uint64_t value = 0;
        std::size_t size = sizeof(value);
        if (sysctlbyname(key, &value, &size, nullptr, 0) != 0) {
            return "?";
        }
        return std::to_string(value);
    };
    std::string text;
    for (const char* level : {"perflevel0", "perflevel1"}) {
        const std::string prefix = std::string("hw.") + level + ".";
        text += std::string(text.empty() ? "" : "; ") + level + ": " +
                number((prefix + "physicalcpu").c_str()) + " cores, L1d " +
                number((prefix + "l1dcachesize").c_str()) + " B, L2 " +
                number((prefix + "l2cachesize").c_str()) + " B";
    }
    return text + "; cache line " + number("hw.cachelinesize") + " B";
}
#endif

std::string compiler() {
#if defined(__clang__)
    std::string version = "clang " __clang_version__;
#elif defined(__GNUC__)
    std::string version = "gcc " __VERSION__;
#else
    std::string version = "unknown";
#endif
    version.erase(version.find_last_not_of(' ') + 1);  // clang ends it with a space
    return version;
}

std::string standard_library() {
#if defined(_LIBCPP_VERSION)
    return "libc++ " + std::to_string(_LIBCPP_VERSION);
#elif defined(__GLIBCXX__)
    return "libstdc++ " + std::to_string(__GLIBCXX__);
#else
    return "unknown";
#endif
}

// macOS can neither pin a thread to a core nor fix the clock. The highest QoS
// class keeps the benchmark on the performance cores (DESIGN.md §6).
std::string raise_priority() {
#if defined(__APPLE__)
    return pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) == 0
               ? "QOS_CLASS_USER_INTERACTIVE"
               : "failed to set QoS";
#else
    return "none (pin with taskset on Linux)";
#endif
}

}  // namespace

int main(int argc, char** argv) {
    // Remove --smoke before Google Benchmark parses the rest.
    bool smoke = false;
    int kept = 1;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke") == 0) {
            smoke = true;
        } else {
            argv[kept++] = argv[i];
        }
    }
    argc = kept;

    // From 2^12 vertices, which fit in L1 and L2, to 2^22, which only fit in
    // DRAM (DESIGN.md §6). Above 2^18 every size is measured: that is where
    // the per-vertex arrays outgrow the caches.
    static constexpr unsigned full_sizes[] = {12, 14, 16, 18, 19, 20, 21, 22};
    static constexpr unsigned smoke_sizes[] = {8};
    const std::span<const unsigned> sizes = smoke ? std::span<const unsigned>(smoke_sizes)
                                                  : std::span<const unsigned>(full_sizes);
    elemgraph::bench::register_bfs(sizes);
    elemgraph::bench::register_connectivity(sizes);

    benchmark::AddCustomContext("cpu_model", cpu_model());
#if defined(__APPLE__)
    benchmark::AddCustomContext("apple_core_caches", apple_core_caches());
#endif
    benchmark::AddCustomContext("compiler", compiler());
    benchmark::AddCustomContext("standard_library", standard_library());
    benchmark::AddCustomContext("priority", raise_priority());
#ifdef NDEBUG
    benchmark::AddCustomContext("elemgraph_assertions", "off");
#else
    benchmark::AddCustomContext("elemgraph_assertions", "on");
    std::cerr << "warning: built without NDEBUG; these timings include assertions\n";
#endif
    benchmark::AddCustomContext("inputs", smoke ? "smoke" : "full");

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
