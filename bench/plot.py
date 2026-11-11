#!/usr/bin/env python3
"""Figures and tables for the README from one results file of bench/run.sh.

    nix develop .#plot -c python3 bench/plot.py bench/results/m2/2026-11-11.json

writes two figures next to the JSON, each in a light and a dark variant:

    <date>-q1.svg, <date>-q1-dark.svg   BFS: adjacency list vs. CSR
    <date>-q4.svg, <date>-q4-dark.svg   chain decomposition vs. lowpoint

With --tables it also prints the result tables as Markdown. All values are
medians of the repetitions.
"""

import argparse
import json
import math
import re
from pathlib import Path

import matplotlib

matplotlib.use("svg")
import matplotlib.pyplot as plt  # noqa: E402  (after choosing the backend)

CLASSES = [("grid", "grid"), ("gnm", "G(n,m)"), ("rmat", "R-MAT")]

# Colors by role, from a palette checked for color-vision deficiencies; the
# dark variant uses the same hues, stepped for a dark background.
THEMES = {
    "light": {
        "surface": "#fcfcfb",
        "ink": "#0b0b0b",
        "secondary": "#52514e",
        "muted": "#898781",
        "grid": "#e1e0d9",
        "baseline": "#c3c2b7",
        "series": ["#2a78d6", "#eb6834", "#1baf7a"],
    },
    "dark": {
        "surface": "#1a1a19",
        "ink": "#f0efec",
        "secondary": "#c3c2b7",
        "muted": "#898781",
        "grid": "#2c2c2a",
        "baseline": "#383835",
        "series": ["#3987e5", "#d95926", "#199e70"],
    },
}
FONT = "system-ui, -apple-system, 'Segoe UI', Helvetica, Arial, sans-serif"

# Bytes per vertex that BFS reads in random order: distance and parent, plus
# the start of the vertex's edges, which is 4 bytes in offsets for CSR and a
# 24-byte std::vector header for the adjacency list.
RANDOM_BYTES_PER_VERTEX = {"csr_graph": 12, "adjacency_list": 32}


def load(path):
    data = json.loads(Path(path).read_text())
    medians = {}
    counters = {}
    for b in data["benchmarks"]:
        if b.get("aggregate_name") == "median":
            name = b["run_name"].removesuffix("/real_time")
            medians[name] = b["real_time"]
            counters[name] = b
    return data["context"], medians, counters


def sizes_of(medians, prefix):
    return sorted({int(name.rsplit("/", 1)[1]) for name in medians if name.startswith(prefix)})


def largest_core_cache(context):
    """The largest cache of the core the benchmark ran on, as (name, bytes)."""
    # On Apple silicon, Google Benchmark reports the efficiency cores' caches;
    # bench/main.cpp records the performance cores' caches separately.
    m = re.search(r"perflevel0: .*?L2 (\d+) B", context.get("apple_core_caches", ""))
    if m:
        return "L2", int(m.group(1))
    caches = context.get("caches", [])
    if not caches:
        return None
    largest = max(caches, key=lambda c: c["size"])
    return f"L{largest['level']}", largest["size"]


def style(ax, t):
    ax.set_facecolor(t["surface"])
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(t["baseline"])
        ax.spines[side].set_linewidth(0.75)
    ax.tick_params(length=0, labelcolor=t["secondary"], labelsize=9, pad=6)
    ax.grid(axis="y", color=t["grid"], linewidth=0.75)
    ax.set_axisbelow(True)


def plot_lines(ax, t, sizes, ratio):
    """One line per graph class; ratio(class, size) gives the y value."""
    for (key, label), color in zip(CLASSES, t["series"]):
        ax.plot(sizes, [ratio(key, s) for s in sizes], color=color, linewidth=1.5,
                solid_joinstyle="round", solid_capstyle="round", marker="o", markersize=6,
                markeredgecolor=t["surface"], markeredgewidth=1.5, label=label, zorder=3)
    ax.axhline(1, color=t["baseline"], linewidth=0.75, zorder=2)
    ax.set_xticks(sizes)
    ax.set_xlabel("vertices (log₂ n)", color=t["secondary"], fontsize=9)


def legend(ax, t, loc):
    ax.legend(loc=loc, frameon=False, fontsize=9, labelcolor=t["secondary"], handlelength=1.6)


def save(fig, path, t):
    fig.savefig(path, format="svg", facecolor=t["surface"], bbox_inches="tight",
                metadata={"Date": None})
    plt.close(fig)
    # Text stays text in the SVG; let the viewer's sans-serif font draw it.
    svg = path.read_text()
    svg = re.sub(r"font-family:[^;\"]*", f"font-family:{FONT}", svg)
    path.write_text(svg)


def figure_q1(context, medians, sizes, t, path):
    fig, ax = plt.subplots(figsize=(6.4, 3.6))
    fig.set_facecolor(t["surface"])
    style(ax, t)

    cache = largest_core_cache(context)
    if cache:
        # The sizes at which CSR's randomly read data fits in the cache and
        # the adjacency list's does not.
        name, size = cache
        low = math.log2(size / RANDOM_BYTES_PER_VERTEX["adjacency_list"])
        high = math.log2(size / RANDOM_BYTES_PER_VERTEX["csr_graph"])
        if low < sizes[-1] and high > sizes[0]:
            ax.axvspan(max(low, sizes[0]), min(high, sizes[-1]), color=t["muted"], alpha=0.12,
                       linewidth=0, zorder=1)
            # Halfway up, between the lines that rise into the band and the
            # grid line below it.
            ax.text((max(low, sizes[0]) + min(high, sizes[-1])) / 2, 0.5,
                    f"CSR fits in\n{size >> 20} MB {name},\nadjacency\nlist does not",
                    transform=ax.get_xaxis_transform(), ha="center", va="center", fontsize=8,
                    color=t["secondary"], linespacing=1.3)

    def ratio(c, s):
        return (medians[f"bfs/flat/adjacency_list/{c}/{s}"] /
                medians[f"bfs/flat/csr_graph/{c}/{s}"])

    plot_lines(ax, t, sizes, ratio)
    ax.set_ylabel("adjacency list ÷ CSR", color=t["secondary"], fontsize=9)
    ax.set_title("BFS time on the adjacency list relative to CSR", loc="left", fontsize=10,
                 color=t["ink"], pad=10)
    legend(ax, t, "upper left")
    save(fig, path, t)


def figure_q4(medians, sizes, t, path):
    fig, axes = plt.subplots(1, 2, figsize=(8.8, 3.6), sharey=True)
    fig.set_facecolor(t["surface"])

    def after_search(c, s):
        return (medians[f"connectivity/chains/{c}/{s}"] /
                medians[f"connectivity/lowpoint/{c}/{s}"])

    def with_search(c, s):
        dfs = medians[f"connectivity/dfs/{c}/{s}"]
        return ((dfs + medians[f"connectivity/chains/{c}/{s}"]) /
                (dfs + medians[f"connectivity/lowpoint/{c}/{s}"]))

    for ax, ratio, title in ((axes[0], after_search, "After the search"),
                             (axes[1], with_search, "Including the search (DFS)")):
        style(ax, t)
        plot_lines(ax, t, sizes, ratio)
        ax.set_title(title, loc="left", fontsize=10, color=t["ink"], pad=10)
        for y, text, va in ((1.0, "  lowpoint faster ↑", "bottom"),
                            (1.0, "  chain decomposition faster ↓", "top")):
            ax.text(sizes[0], y, text, ha="left", va=va, fontsize=8, color=t["muted"])
    axes[0].set_ylabel("chain decomposition ÷ lowpoint", color=t["secondary"], fontsize=9)
    legend(axes[1], t, "upper right")
    fig.tight_layout(w_pad=3)
    save(fig, path, t)


def markdown_tables(medians, counters, sizes_q1, sizes_q4):
    def time(us):
        return f"{us:,.1f}" if us < 1000 else f"{us:,.0f}"

    labels = dict(CLASSES)
    lines = ["| Graph | log2 n | m | Bridges | Cut vertices | DFS | Lowpoint | Chains |"
             " Chains / lowpoint | With DFS |",
             "|-------|-------:|--:|--------:|-------------:|----:|---------:|-------:|"
             "------------------:|---------:|"]
    for c, _ in CLASSES:
        for s in sizes_q4:
            def k(part):
                return f"connectivity/{part}/{c}/{s}"
            dfs, low, chains = medians[k("dfs")], medians[k("lowpoint")], medians[k("chains")]
            info = counters[k("dfs")]
            r = chains / low
            lines.append(
                f"| {labels[c]} | {s} | {int(info['m']):,} | {int(info['bridges']):,} | "
                f"{int(info['cut_vertices']):,} | {time(dfs)} | {time(low)} | {time(chains)} | "
                f"{f'**{r:.2f}**' if r < 1 else f'{r:.2f}'} | {(dfs + chains) / (dfs + low):.2f} |")
    lines += ["", "| log2 n | grid | G(n,m) | R-MAT |", "|-------:|-----:|-------:|------:|"]
    for s in sizes_q1:
        cells = [medians[f"bfs/flat/adjacency_list/{c}/{s}"] /
                 medians[f"bfs/flat/csr_graph/{c}/{s}"] for c, _ in CLASSES]
        lines.append(f"| {s} | " + " | ".join(f"{r:.2f}" for r in cells) + " |")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("results", type=Path, help="a JSON file written by bench/run.sh")
    parser.add_argument("--tables", action="store_true", help="also print Markdown tables")
    parser.add_argument("--min-log-n", type=int, default=14,
                        help="smallest size in the figures (default 14: the times with 2^12 "
                             "vertices vary too much between runs)")
    args = parser.parse_args()

    plt.rcParams["svg.fonttype"] = "none"
    plt.rcParams["svg.hashsalt"] = "elemgraph"  # the same SVG for the same data
    plt.rcParams["font.family"] = "sans-serif"

    context, medians, counters = load(args.results)
    sizes_q1 = sizes_of(medians, "bfs/flat/csr_graph/")
    sizes_q4 = sizes_of(medians, "connectivity/dfs/")
    stem = args.results.with_suffix("")
    for theme, suffix in (("light", ""), ("dark", "-dark")):
        t = THEMES[theme]
        if sizes_q1:
            path = Path(f"{stem}-q1{suffix}.svg")
            figure_q1(context, medians, [s for s in sizes_q1 if s >= args.min_log_n], t, path)
            print(f"wrote {path}")
        if sizes_q4:
            path = Path(f"{stem}-q4{suffix}.svg")
            figure_q4(medians, [s for s in sizes_q4 if s >= args.min_log_n], t, path)
            print(f"wrote {path}")
    if args.tables:
        print()
        print(markdown_tables(medians, counters, sizes_q1, sizes_q4))


if __name__ == "__main__":
    main()
