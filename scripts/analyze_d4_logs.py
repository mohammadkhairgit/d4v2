#!/usr/bin/env python3
"""Parse d4 logs into simple CSV files and create plots from those CSV files."""

import csv
import json
import re
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "instancesTest/cnfs_and_alternatives/extra/results/car_confidential/dimacsOutputPath"
D4_FILE = RESULTS / "to_ddnnf_output_after_optimization.log"
DDNNIFE_FILE = RESULTS / "ddnnf_statistics_output_after_optimization.log"
OUTPUT_DIRECTORY = RESULTS / "plots/log_analysis"
TEXT_TO_ADD = "BestBest"
OUTPUT_FILE_PREFIX = re.sub(r"[^A-Za-z0-9._-]+", "_", TEXT_TO_ADD).strip("_")
INSTANCE_RANGES = ((1, 15), (16, 26))
SHOW_BAR_VALUES = True
BAR_VALUE_FONT_SIZE = 14

# List the compact method names to include in the plots and derived tables.
METHODS_TO_PLOT = [
    # "branch-min_cF",
    # "branch-min_cT",
    # "branch-min smallerClauseSize_cF",
    "branch-min smallerClauseSize_cT",
    # "literal-sum-over-branch-avg_cF",
    # "literal-sum-over-branch-avg_cT",
    # "literal-sum-over-branch-avg smallerClauseSize_cF",
    # "literal-sum-over-branch-avg smallerClauseSize_cT",
    # "literal-sum_cF",
    # "literal-sum_cT",
    # "none_cF",
    # "none_cF_opt",
    "none_cT",
    "none_cT_opt",
]

INSTANCE_NUMBERS = {
    "1-Car_with_all_features_unsliced": 1,
    "2-Car_with_all_features_unsliced": 2,
    "3-Car_with_all_features_unsliced": 3,
    "5-Car_with_all_features_unsliced": 4,
    "FinancialServices01-Nieke2018-2018-03-26": 5,
    "FinancialServices01-Nieke2018-2018-04-23": 6,
    "FinancialServices01-Nieke2018-2018-05-09": 7,
    "automotive01-Kowal2016": 8,
    "automotive2-Knüppel2017-2_1": 9,
    "automotive2-Knüppel2017-2_2": 10,
    "automotive2-Knüppel2017-2_3": 11,
    "automotive2-Knüppel2017-2_4": 12,
    "cdl-adder-Knüppel2017": 13,
    "cdl-aim711-Knüppel2017": 14,
    "embtoolkit-Knüppel2017": 15,
    "firefoxcve-VarelaVaca2020-2002-2436": 16,
    "firefoxcve-VarelaVaca2020-2009-1308": 17,
    "firefoxcve-VarelaVaca2020-2011-1300": 18,
    "firefoxcve-VarelaVaca2020-2012-0471": 19,
    "firefoxcve-VarelaVaca2020-2012-3987": 20,
    "firefoxcve-VarelaVaca2020-2013-0783": 21,
    "firefoxcve-VarelaVaca2020-2015-0819": 22,
    "firefoxcve-VarelaVaca2020-2017-5393": 23,
    "firefoxcve-VarelaVaca2020-2020-12396": 24,
    "linux-Knüppel2017-2.6.33.3": 25,
    "pc-richmond-Sprey2020": 26,
    "7-Car_with_all_features_unsliced": 27,
}

ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
BOUNDARY = "Run configuration:"
CACHE_SHORT_NAMES = {"true": "T", "false": "F"}
PREPROCESSING_SHORT_NAMES = {"true": "opt", "false": ""}


def clean(line):
    return ANSI.sub("", line).strip()


def method(fields):
    clause_scoring = fields.get("clause scoring", "unknown")
    cache = CACHE_SHORT_NAMES.get(
        fields.get("cache activated", "unknown").lower(), "?"
    )
    preprocessing = fields.get("preprocessing and partitioning", "unknown").lower()
    short_name = f"{clause_scoring}_c{cache}"
    if clause_scoring == "none":
        short_name += f"_{PREPROCESSING_SHORT_NAMES.get(preprocessing, preprocessing)}"
    return short_name.rstrip("_")


def block_fields(block):
    fields = {}
    for line in block:
        # Configuration metadata ends at the command line. Lines after the
        # command can already be preliminary metadata for the next run.
        if line.startswith("command:"):
            break
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        fields[key.strip().lower()] = value.strip()
    return fields


def find_value(block, pattern):
    for line in block:
        match = re.search(pattern, line)
        if match:
            return float(match.group(1))
    return None


def find_nodes(block):
    for line in block:
        start = line.find('{"nodes"')
        if start == -1:
            continue
        try:
            data, _ = json.JSONDecoder().raw_decode(line[start:])
            return int(data["nodes"]["total"])
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            return None
    return None


def output_path(file_name):
    return OUTPUT_DIRECTORY / f"{OUTPUT_FILE_PREFIX}_{file_name}"


def write_dataframe(data, path):
    data.to_csv(path, index=False)


def format_bar_value(value):
    formatted = f"{value:.3g}"
    match = re.fullmatch(r"(-?[0-9]+(?:\.[0-9]+)?)e([+-]?\d+)", formatted)
    if not match:
        return formatted
    mantissa, exponent = match.groups()
    return rf"${mantissa}\times10^{{{int(exponent)}}}$"


def add_bar_values(axis):
    if not SHOW_BAR_VALUES:
        return
    for container in axis.containers:
        axis.bar_label(
            container,
            labels=[format_bar_value(value) for value in container.datavalues],
            padding=2,
            fontsize=BAR_VALUE_FONT_SIZE,
        )


def parse_csv(log_file, csv_file, value_name, pattern=None, nodes=False):
    """Scan one complete log and write rows for one requested value."""
    rows = []
    block = []

    def parse_block(lines):
        fields = block_fields(lines)
        file_name = fields.get("file name")
        if not file_name or file_name not in INSTANCE_NUMBERS:
            return
        value = find_nodes(lines) if nodes else find_value(lines, pattern)
        if value is None:
            return
        if not any("SUCCESS:" in line for line in lines):
            return
        rows.append(
            {
                "file_name": file_name,
                "instance_number": INSTANCE_NUMBERS[file_name],
                "method": method(fields),
                value_name: value,
            }
        )

    for raw_line in log_file.read_text(encoding="utf-8", errors="replace").splitlines():
        line = clean(raw_line)
        if BOUNDARY in line:
            parse_block(block)
            block = []
        else:
            block.append(line)

    parse_block(block)
    csv_file.parent.mkdir(parents=True, exist_ok=True)
    with csv_file.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(
            output,
            fieldnames=["file_name", "instance_number", "method", value_name],
        )
        writer.writeheader()
        writer.writerows(rows)
    return rows


def plot_csv(csv_file, value_name, output_name, method_colors):
    import matplotlib.pyplot as plt
    from matplotlib.ticker import ScalarFormatter
    import pandas as pd

    def use_log_y_axis(axis):
        axis.set_yscale("log")
        formatter = ScalarFormatter()
        formatter.set_scientific(False)
        formatter.set_useOffset(False)
        axis.yaxis.set_major_formatter(formatter)

    data = pd.read_csv(csv_file)
    if METHODS_TO_PLOT:
        data = data[data["method"].isin(METHODS_TO_PLOT)]
    if data.empty:
        return

    present_methods = set(data["method"])
    method_order = [name for name in METHODS_TO_PLOT if name in present_methods]
    method_order.extend(
        sorted(present_methods.difference(method_order))
    )

    by_instance = data[["instance_number", "file_name", "method", value_name]]
    write_dataframe(
        by_instance, output_path(f"{output_name}_by_instance.csv")
    )
    for first_instance, last_instance in INSTANCE_RANGES:
        selected = by_instance[
            by_instance["instance_number"].between(first_instance, last_instance)
        ]
        if selected.empty:
            continue
        pivot = selected.pivot_table(
            index="instance_number",
            columns="method",
            values=value_name,
            aggfunc="last",
        ).reindex(columns=method_order)
        axis = pivot.plot(
            kind="bar",
            figsize=(14, 6),
            title=f"{value_name} by instance ({first_instance}-{last_instance})",
            color=[method_colors[name] for name in pivot.columns],
        )
        axis.set_xlabel("Instance number")
        axis.set_ylabel(value_name)
        use_log_y_axis(axis)
        add_bar_values(axis)
        axis.figure.tight_layout()
        axis.figure.savefig(
            output_path(
                f"{output_name}_by_instance_{first_instance}_{last_instance}.png"
            ),
            dpi=160,
        )
        plt.close(axis.figure)

    aggregates = data.groupby("method")[value_name].agg(["min", "max", "mean"])
    aggregates = aggregates.rename(columns={"mean": "avg"}).reset_index()
    aggregate_table = aggregates.melt(
        id_vars="method", var_name="metric", value_name="value"
    )
    write_dataframe(
        aggregate_table, output_path(f"{output_name}_aggregates.csv")
    )
    pivot = aggregate_table.pivot(
        index="metric", columns="method", values="value"
    ).reindex(columns=method_order)
    axis = pivot.plot(
        kind="bar",
        figsize=(8, 6),
        title=f"{value_name} aggregates",
        color=[method_colors[name] for name in pivot.columns],
    )
    axis.set_xlabel("Metric")
    axis.set_ylabel(value_name)
    use_log_y_axis(axis)
    add_bar_values(axis)
    axis.figure.tight_layout()
    axis.figure.savefig(
        output_path(f"{output_name}_aggregates.png"), dpi=160
    )
    plt.close(axis.figure)

    counts = Counter(data["method"])
    count_table = pd.DataFrame(
        [
            {"method": name, "finished_count": counts[name]}
            for name in method_order
        ]
    )
    write_dataframe(
        count_table, output_path(f"{output_name}_finished_counts.csv")
    )
    count_plot = count_table.set_index("method").T
    axis = count_plot.plot(
        kind="bar",
        figsize=(10, 6),
        title=f"{output_name} finished instances",
        color=[method_colors[name] for name in count_plot.columns],
    )
    axis.set_xlabel("")
    axis.set_ylabel("Finished instances")
    axis.legend(title="Method", fontsize="small")
    add_bar_values(axis)
    axis.figure.tight_layout()
    axis.figure.savefig(
        output_path(f"{output_name}_finished_counts.png"), dpi=160
    )
    plt.close(axis.figure)


def main():
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    d4_rows = parse_csv(
        D4_FILE,
        output_path("d4_time.csv"),
        "d4_time",
        pattern=r"c Final time:\s*([0-9]+(?:\.[0-9]+)?)",
    )
    ddnnife_time_rows = parse_csv(
        DDNNIFE_FILE,
        output_path("ddnnife_time.csv"),
        "ddnnife_time",
        pattern=r"Elapsed time for parsing, and overall count:\s*([0-9]+(?:\.[0-9]+)?)s",
    )
    node_rows = parse_csv(
        DDNNIFE_FILE,
        output_path("node_count.csv"),
        "node_count",
        nodes=True,
    )

    import matplotlib.pyplot as plt

    methods = sorted(
        {
            row["method"]
            for rows in (d4_rows, ddnnife_time_rows, node_rows)
            for row in rows
        }
    )
    palette = plt.colormaps["tab20"].resampled(max(1, len(methods)))
    method_colors = {
        method_name: palette(index)
        for index, method_name in enumerate(methods)
    }

    plot_csv(
        output_path("d4_time.csv"), "d4_time", "d4_time", method_colors
    )
    plot_csv(
        output_path("ddnnife_time.csv"),
        "ddnnife_time",
        "ddnnife_time",
        method_colors,
    )
    plot_csv(output_path("node_count.csv"), "node_count", "node_count", method_colors)
    print(
        f"Parsed {len(d4_rows)} d4 times, "
        f"{len(ddnnife_time_rows)} ddnnife times, and "
        f"{len(node_rows)} node counts."
    )


if __name__ == "__main__":
    main()
