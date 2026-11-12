#!/usr/bin/env python3
"""Figures and tables for the README from results files of bench/run.sh.

    nix develop .#plot -c python3 bench/plot.py bench/results/m2/2026-11-11.json

writes two figures next to the JSON, each in a light and a dark variant:

    <date>-q1.svg, <date>-q1-dark.svg   BFS: adjacency list vs. CSR
    <date>-q4.svg, <date>-q4-dark.svg   chain decomposition vs. lowpoint

With several results files and --out DIR, it writes q1.svg, q4.svg and their
dark variants into DIR instead, with one panel (Q1) or one row (Q4) per file,
to compare machines.

With --tables it also prints the result tables as Markdown, and the hardware
counters per vertex if the run recorded them. All values are medians of the
repetitions.
"""

import argparse
import json
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

# Google Benchmark's names of the counters that bench/run.sh records on Linux.
COUNTERS = [("INSTRUCTIONS", "Instructions", "{:.0f}"), ("CACHE-MISSES", "Cache misses", "{:.2f}"),
            ("BRANCH-MISSES", "Branch misses", "{:.2f}")]


class Run:
    """The medians of one results file."""

    def __init__(self, path):
        data = json.loads(Path(path).read_text())
        self.path = Path(path)
        self.context = data["context"]
        self.medians = {}  # benchmark name -> median time
        self.counters = {}  # benchmark name -> the median entry, with its counters
        for b in data["benchmarks"]:
            if b.get("aggregate_name") == "median":
                name = b["run_name"].removesuffix("/real_time")
                self.medians[name] = b["real_time"]
                self.counters[name] = b

    def sizes(self, prefix, min_log_n=0):
        return sorted({int(name.rsplit("/", 1)[1]) for name in self.medians
                       if name.startswith(prefix) and int(name.rsplit("/", 1)[1]) >= min_log_n})

    def label(self):
        """The CPU's name, e.g. "Apple M2" or "Intel Core i7-10750H"."""
        model = re.sub(r"\((R|TM)\)", "", self.context.get("cpu_model", ""))
        model = re.sub(r"\s+CPU\s+@.*$", "", model)
        return " ".join(model.split()) or self.context.get("machine", self.path.parent.name)

    def has_counters(self):
        return any(key in b for b in self.counters.values() for key, _, _ in COUNTERS)

    def per_vertex(self, name, key):
        b = self.counters[name]
        return b[key] / b["n"]


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


def title(ax, t, text):
    ax.set_title(text, loc="left", fontsize=10, color=t["ink"], pad=10)


def save(fig, path, t):
    fig.savefig(path, format="svg", facecolor=t["surface"], bbox_inches="tight",
                metadata={"Date": None})
    plt.close(fig)
    # Text stays text in the SVG; let the viewer's sans-serif font draw it.
    svg = path.read_text()
    svg = re.sub(r"font-family:[^;\"]*", f"font-family:{FONT}", svg)
    path.write_text(svg)
    print(f"wrote {path}")


def figure_q1(runs, t, path, min_log_n):
    """One panel per run."""
    fig, axes = plt.subplots(1, len(runs), figsize=(6.4 if len(runs) == 1 else 4.4 * len(runs), 3.6),
                             sharey=True, squeeze=False)
    fig.set_facecolor(t["surface"])
    heading = "BFS time on the adjacency list relative to CSR"
    for ax, run in zip(axes[0], runs):
        style(ax, t)

        def ratio(c, s, run=run):
            return (run.medians[f"bfs/flat/adjacency_list/{c}/{s}"] /
                    run.medians[f"bfs/flat/csr_graph/{c}/{s}"])

        plot_lines(ax, t, run.sizes("bfs/flat/csr_graph/", min_log_n), ratio)
        title(ax, t, heading if len(runs) == 1 else run.label())
    axes[0][0].set_ylabel("adjacency list ÷ CSR", color=t["secondary"], fontsize=9)
    legend(axes[0][0], t, "upper left")
    if len(runs) > 1:
        fig.suptitle(heading, x=0.01, ha="left", fontsize=10.5, color=t["ink"])
    fig.tight_layout(w_pad=3)
    save(fig, path, t)


def figure_q4(runs, t, path, min_log_n):
    """One row per run: the time after the search, and including it."""
    fig, axes = plt.subplots(len(runs), 2, figsize=(8.8, 3.6 * len(runs)), sharey=True,
                             squeeze=False)
    fig.set_facecolor(t["surface"])
    for row, run in zip(axes, runs):

        def after_search(c, s, run=run):
            return (run.medians[f"connectivity/chains/{c}/{s}"] /
                    run.medians[f"connectivity/lowpoint/{c}/{s}"])

        def with_search(c, s, run=run):
            dfs = run.medians[f"connectivity/dfs/{c}/{s}"]
            return ((dfs + run.medians[f"connectivity/chains/{c}/{s}"]) /
                    (dfs + run.medians[f"connectivity/lowpoint/{c}/{s}"]))

        sizes = run.sizes("connectivity/dfs/", min_log_n)
        for ax, ratio, text in ((row[0], after_search, "After the search"),
                                (row[1], with_search, "Including the search (DFS)")):
            style(ax, t)
            plot_lines(ax, t, sizes, ratio)
            title(ax, t, text if len(runs) == 1 else f"{run.label()}: {text[0].lower()}{text[1:]}")
        # In the label rather than next to the line at 1, where the data may lie.
        row[0].set_ylabel("chain decomposition ÷ lowpoint\n(above 1: lowpoint is faster)",
                          color=t["secondary"], fontsize=9, linespacing=1.5)
    legend(axes[0][1], t, "upper right")
    fig.tight_layout(w_pad=3, h_pad=2)
    save(fig, path, t)


def markdown_tables(run):
    def time(us):
        return f"{us:,.1f}" if us < 1000 else f"{us:,.0f}"

    def counter_cells(names):
        return [", ".join(fmt.format(run.per_vertex(name, key)) for name in names)
                for key, _, fmt in COUNTERS]

    labels = dict(CLASSES)
    sizes_q4 = run.sizes("connectivity/dfs/")
    sizes_q1 = run.sizes("bfs/flat/csr_graph/")
    counters = run.has_counters()
    lines = []
    if sizes_q4:
        lines += ["| Graph | log2 n | m | Bridges | Cut vertices | DFS | Lowpoint | Chains |"
                  " Chains / lowpoint | With DFS |",
                  "|-------|-------:|--:|--------:|-------------:|----:|---------:|-------:|"
                  "------------------:|---------:|"]
        for c, _ in CLASSES:
            for s in sizes_q4:
                def k(part):
                    return f"connectivity/{part}/{c}/{s}"
                dfs, low = run.medians[k("dfs")], run.medians[k("lowpoint")]
                chains = run.medians[k("chains")]
                info = run.counters[k("dfs")]
                r = chains / low
                lines.append(
                    f"| {labels[c]} | {s} | {int(info['m']):,} | {int(info['bridges']):,} | "
                    f"{int(info['cut_vertices']):,} | {time(dfs)} | {time(low)} | {time(chains)} | "
                    f"{f'**{r:.2f}**' if r < 1 else f'{r:.2f}'} | "
                    f"{(dfs + chains) / (dfs + low):.2f} |")
        if counters:
            lines += ["", "Per vertex, for DFS, lowpoint, chains:", "",
                      "| Graph | log2 n | " + " | ".join(h for _, h, _ in COUNTERS) + " |",
                      "|-------|-------:|" + "--:|" * len(COUNTERS)]
            for c, _ in CLASSES:
                for s in sizes_q4:
                    names = [f"connectivity/{p}/{c}/{s}" for p in ("dfs", "lowpoint", "chains")]
                    lines.append(f"| {labels[c]} | {s} | " + " | ".join(counter_cells(names)) + " |")
        lines.append("")
    if sizes_q1:
        lines += ["| log2 n | grid | G(n,m) | R-MAT |", "|-------:|-----:|-------:|------:|"]
        for s in sizes_q1:
            cells = [run.medians[f"bfs/flat/adjacency_list/{c}/{s}"] /
                     run.medians[f"bfs/flat/csr_graph/{c}/{s}"] for c, _ in CLASSES]
            lines.append(f"| {s} | " + " | ".join(f"{r:.2f}" for r in cells) + " |")
        if counters:
            lines += ["", "Per vertex, for CSR, adjacency list:", "",
                      "| Graph | log2 n | Adjacency list ÷ CSR | " +
                      " | ".join(h for _, h, _ in COUNTERS) + " |",
                      "|-------|-------:|--:|" + "--:|" * len(COUNTERS)]
            for c, _ in CLASSES:
                for s in sizes_q1:
                    names = [f"bfs/flat/{r}/{c}/{s}" for r in ("csr_graph", "adjacency_list")]
                    r = run.medians[names[1]] / run.medians[names[0]]
                    lines.append(f"| {labels[c]} | {s} | {r:.2f} | " +
                                 " | ".join(counter_cells(names)) + " |")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("results", type=Path, nargs="+", help="JSON files written by bench/run.sh")
    parser.add_argument("--out", type=Path,
                        help="the directory for figures that compare the results files")
    parser.add_argument("--tables", action="store_true", help="also print Markdown tables")
    parser.add_argument("--min-log-n", type=int, default=14,
                        help="smallest size in the figures (default 14: the times with 2^12 "
                             "vertices vary too much between runs)")
    args = parser.parse_args()
    if len(args.results) > 1 and args.out is None:
        parser.error("several results files need --out")

    plt.rcParams["svg.fonttype"] = "none"
    plt.rcParams["svg.hashsalt"] = "elemgraph"  # the same SVG for the same data
    plt.rcParams["font.family"] = "sans-serif"

    runs = [Run(path) for path in args.results]
    if args.out is None:
        prefix = f"{args.results[0].with_suffix('')}-"
    else:
        args.out.mkdir(parents=True, exist_ok=True)
        prefix = f"{args.out}/"
    for theme, suffix in (("light", ""), ("dark", "-dark")):
        t = THEMES[theme]
        if all(run.sizes("bfs/flat/csr_graph/") for run in runs):
            figure_q1(runs, t, Path(f"{prefix}q1{suffix}.svg"), args.min_log_n)
        if all(run.sizes("connectivity/dfs/") for run in runs):
            figure_q4(runs, t, Path(f"{prefix}q4{suffix}.svg"), args.min_log_n)
    if args.tables:
        for run in runs:
            print(f"\n{run.path}\n")
            print(markdown_tables(run))


if __name__ == "__main__":
    main()
