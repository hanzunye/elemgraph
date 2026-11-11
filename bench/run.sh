#!/usr/bin/env bash
# Runs the full benchmark suite the same way on every machine (DESIGN.md §6):
#
#   bench/run.sh <machine> [Google Benchmark flags]
#
# for example `bench/run.sh m2`, or `bench/run.sh x86-laptop
# --benchmark_filter=connectivity/` for one suite. It builds and tests the
# release preset in the Nix dev shell, runs every benchmark 10 times, and
# writes bench/results/<machine>/<date>.json (<date>-2.json and so on for
# later runs on the same day). Flags given here override the defaults.
#
# The JSON records what the timings depend on: the commit, and the power
# source on macOS or the core, governor and turbo setting on Linux. The script
# only checks the machine's settings; changing them needs root, so it prints
# the commands instead.
#
# Linux: the run is pinned to one CPU (ELEMGRAPH_CPU, default 2). If the kernel
# allows it, every benchmark also reports hardware counters, per iteration
# (ELEMGRAPH_PERF_COUNTERS, libpfm names, default below).
set -euo pipefail

if [[ $# -lt 1 || $1 == -* ]]; then
    echo "usage: bench/run.sh <machine> [Google Benchmark flags]" >&2
    exit 2
fi
machine=$1
shift

cd "$(dirname "${BASH_SOURCE[0]}")/.."

warn() {
    echo "warning: $*" >&2
}

# The commit the results belong to. Uncommitted changes to tracked files
# would make the results unreproducible, so they are marked.
commit=$(git rev-parse --short HEAD)
if [[ -n $(git status --porcelain --untracked-files=no) ]]; then
    warn "uncommitted changes: the results will not match commit $commit"
    commit="$commit-dirty"
fi

out_dir=bench/results/$machine
out=$out_dir/$(date +%F).json
run=2
while [[ -e $out ]]; do
    out=$out_dir/$(date +%F)-$run.json
    run=$((run + 1))
done

# key=value pairs for --benchmark_context; values must not contain commas.
context=("git_commit=$commit" "machine=$machine")
pin=()    # command that runs the benchmark pinned to one CPU, if any
flags=()  # extra Google Benchmark flags

case $(uname -s) in
Darwin)
    # QoS is set by the benchmark itself (bench/main.cpp); macOS offers no
    # core pinning and no fixed clock.
    power=$(pmset -g batt | head -1 | sed -e "s/.*'\(.*\)'.*/\1/")
    context+=("power=$power")
    [[ $power == "AC Power" ]] || warn "not on AC power ($power)"
    ;;
Linux)
    cpu=${ELEMGRAPH_CPU:-2}
    cpu_dir=/sys/devices/system/cpu/cpu$cpu
    [[ -d $cpu_dir ]] || { echo "error: there is no CPU $cpu" >&2; exit 1; }
    if command -v taskset >/dev/null; then
        pin=(taskset -c "$cpu")
        context+=("pinned_cpu=$cpu")
    else
        warn "taskset not found: the run is not pinned to a CPU"
        context+=("pinned_cpu=none")
    fi
    # With SMT, the other hardware thread of the same core shares its caches.
    siblings=$(cat "$cpu_dir/topology/thread_siblings_list" 2>/dev/null || echo "$cpu")
    context+=("cpu_siblings=${siblings//,/;}")

    governor=$(cat "$cpu_dir/cpufreq/scaling_governor" 2>/dev/null || echo unknown)
    context+=("governor=$governor")
    if [[ $governor != performance ]]; then
        warn "CPU $cpu uses the governor '$governor'. To fix it, as root:"
        echo "  echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor" >&2
    fi

    turbo=unknown
    if [[ -r /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then  # Intel
        turbo=on
        [[ $(cat /sys/devices/system/cpu/intel_pstate/no_turbo) == 1 ]] && turbo=off
        turbo_fix="echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo"
    elif [[ -r /sys/devices/system/cpu/cpufreq/boost ]]; then  # AMD and others
        turbo=on
        [[ $(cat /sys/devices/system/cpu/cpufreq/boost) == 0 ]] && turbo=off
        turbo_fix="echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost"
    fi
    context+=("turbo=$turbo")
    if [[ $turbo == on ]]; then
        warn "turbo is on, so the clock varies. To turn it off, as root:"
        echo "  $turbo_fix" >&2
    fi

    for supply in /sys/class/power_supply/{AC,ADP}*/online; do
        if [[ -r $supply ]]; then
            power=battery
            [[ $(cat "$supply") == 1 ]] && power=AC
            context+=("power=$power")
            [[ $power == AC ]] || warn "not on AC power"
            break
        fi
    done

    # Counting the events of one's own process needs perf_event_paranoid <= 2.
    paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)
    if ((paranoid <= 2)); then
        counters=${ELEMGRAPH_PERF_COUNTERS:-CYCLES,INSTRUCTIONS,CACHE-MISSES,BRANCH-MISSES}
        flags+=("--benchmark_perf_counters=$counters")
    else
        warn "perf_event_paranoid is $paranoid, so no hardware counters. To allow them, as root:"
        echo "  sudo sysctl kernel.perf_event_paranoid=2" >&2
    fi
    ;;
*)
    warn "unknown system $(uname -s): nothing about the machine is checked"
    ;;
esac

echo "Building and testing the release preset..."
nix develop --command cmake --workflow --preset release

mkdir -p "$out_dir"
echo "Writing $out"
# ${array[@]+...} keeps empty arrays working with set -u in bash 3.2 (macOS).
${pin[@]+"${pin[@]}"} build/release/bench/elemgraph_bench \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true \
    --benchmark_out_format=json \
    --benchmark_out="$out" \
    --benchmark_context="$(IFS=,; echo "${context[*]}")" \
    ${flags[@]+"${flags[@]}"} \
    "$@"
echo "Done: $out"
