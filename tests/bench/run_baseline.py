#!/usr/bin/env python3
"""Run every Sub0Pub benchmark variant and print a combined Markdown report.

For each benchmark executable this collects:
  * ns/op   - wall-clock time from nanobench (machine- and noise-dependent)
  * instr/op - exact instructions per operation under valgrind --tool=callgrind
               (deterministic for a given compiler/flags; use as the regression bar)

Usage:
  cmake --preset default && cmake --build --preset default
  python3 tests/bench/run_baseline.py [build/tests] [--budgets tests/bench/budgets.json] > report.md
Options:
  --no-callgrind   skip instruction counting (valgrind not installed / Windows)
  --no-timing      skip wall-clock timing
  --budgets FILE   fail if selected instruction counts exceed recorded limits
"""
import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import OrderedDict

INSTR_ITERATIONS = 10000  # must match bench::cInstrIterations

CORE_VARIANTS = OrderedDict([
    ("Sub0Pub_Bench", "Direct (default)"),
    ("Sub0Pub_Bench_Checked", "Direct + check"),
    ("Sub0Pub_Bench_Full", "Full (snapshot, cancel, filter)"),
    ("Sub0Pub_Bench_ThreadSafe", "ThreadSafe"),
])
IPC_VARIANTS = OrderedDict([("Sub0Pub_Bench_Ipc", "Default")])
AXES_VARIANTS = OrderedDict([("Sub0Pub_Bench_Axes", "Configuration axes")])

ROW = re.compile(r"^\|\s*([\d.,]+|-)\s*\|.*\|\s*(?::\w+:\s*)?`([^`]+)`")
TITLE = re.compile(r"^\|\s*ns/op\s*\|.*\|\s*([^|]+?)\s*$")


def find_exe(build_dir, name, required=True):
    for candidate in (os.path.join(build_dir, name), os.path.join(build_dir, "Release", name + ".exe")):
        if os.path.exists(candidate):
            return candidate
    if required:
        sys.exit(f"benchmark executable not found: {name} in {build_dir}")
    return None


def run_timing(exe):
    """Return ({(section, scenario): ns/op}, header lines)."""
    out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout
    results, section, header = OrderedDict(), "", []
    for line in out.splitlines():
        m = TITLE.match(line)
        if m:
            section = m.group(1)
            continue
        m = ROW.match(line)
        if m:
            value = m.group(1).replace(",", "")
            results[(section, m.group(2))] = None if value == "-" else float(value)
        elif line and not line.startswith("|") and not line.startswith("=="):
            header.append(line)
    return results, header


def run_callgrind(exe):
    """Return {(section, scenario): instructions/op} from per-scenario callgrind dumps."""
    results = OrderedDict()
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["valgrind", "--tool=callgrind", "--callgrind-out-file=" + os.path.join(tmp, "cg.%p"), exe],
                       capture_output=True, text=True, check=True)
        dumps = []
        for path in glob.glob(os.path.join(tmp, "cg.*")):
            label, total, part = None, None, 0
            with open(path) as f:
                for line in f:
                    if line.startswith("desc: Trigger: Client Request: "):
                        label = line.split("Client Request: ", 1)[1].strip()
                    elif line.startswith("part: "):
                        part = int(line.split()[1])
                    elif line.startswith("summary: ") or line.startswith("totals: "):
                        total = int(line.split()[1])
            if label and total is not None:
                dumps.append((part, label, total))
        for _, label, total in sorted(dumps):
            section, _, scenario = label.partition(" | ")
            results[(section, scenario)] = total / INSTR_ITERATIONS
    return results


def fmt(value, digits):
    return "-" if value is None else f"{value:.{digits}f}"


def report(title, variants, build_dir, timing, callgrind):
    columns = OrderedDict()
    header = []
    for exe_name, label in variants.items():
        exe = find_exe(build_dir, exe_name)
        t, h = run_timing(exe) if timing else ({}, [])
        c = run_callgrind(exe) if callgrind else {}
        header = header or h
        columns[label] = (t, c)

    keys = []
    for t, c in columns.values():
        for k in list(t.keys()) + list(c.keys()):
            if k not in keys:
                keys.append(k)

    print(f"## {title}\n")
    instruction_counts = {}
    labels = list(columns.keys())
    metrics = ([("instr/op", 1)] if callgrind else []) + ([("ns/op", 2)] if timing else [])
    for metric, digits in metrics:
        print(f"### {metric}\n")
        print("| Scenario | " + " | ".join(labels) + " |")
        print("|---|" + "---:|" * len(labels))
        section = None
        for key in keys:
            if key[0] != section:
                section = key[0]
                print(f"| **{section}** |" + " |" * len(labels))
            cells = []
            for label in labels:
                t, c = columns[label]
                cells.append(fmt((c if metric == "instr/op" else t).get(key), digits))
                if metric == "instr/op" and key in c:
                    instruction_counts[(title, key[0], key[1], label)] = c[key]
            print(f"| {key[1]} | " + " | ".join(cells) + " |")
        print()
    return instruction_counts


def check_budgets(measured, budgets):
    """Selected, stable Callgrind instruction counts are release gates; omitted rows remain report-only."""
    errors = []
    seen = set()
    for row in budgets["measurements"]:
        key = (row["suite"], row["section"], row["scenario"], row["variant"])
        if key in seen:
            errors.append(f"duplicate budget: {' / '.join(key)}")
            continue
        seen.add(key)
        actual = measured.get(key)
        limit = row["max_instr_per_op"]
        if actual is None:
            errors.append(f"missing measurement: {' / '.join(key)}")
        # Budgets are recorded from the report's one-decimal values. Callgrind totals divided by 10,000
        # iterations can carry a smaller fractional remainder even for the unchanged binary.
        elif round(actual, 1) > limit + 1e-6:
            errors.append(f"{' / '.join(key)}: {actual:.1f} > {limit:.1f} instr/op")
    if not seen:
        errors.append("no benchmark budgets selected")
    return errors


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("build_dir", nargs="?", default=os.path.join("build", "tests"))
    ap.add_argument("--no-callgrind", action="store_true")
    ap.add_argument("--no-timing", action="store_true")
    ap.add_argument("--budgets")
    args = ap.parse_args()
    build_dir = args.build_dir
    timing = not args.no_timing
    callgrind = not args.no_callgrind and shutil.which("valgrind") is not None
    if args.budgets and not callgrind:
        ap.error("--budgets requires Valgrind/Callgrind")

    info = subprocess.run([find_exe(build_dir, "Sub0Pub_Bench")], capture_output=True, text=True).stdout
    print("# Sub0Pub benchmark report\n")
    print("```")
    for line in info.splitlines():
        if line.startswith("|") or not line.strip():
            break
        print(line)
    print("```\n")
    if callgrind:
        print(f"instr/op: callgrind instruction count / {INSTR_ITERATIONS} iterations "
              "(includes ~3 instructions of loop overhead).\n")
    measured = {}
    measured.update(report("Core publish/subscribe by policy", CORE_VARIANTS, build_dir, timing, callgrind))
    measured.update(report("IPC end-to-end", IPC_VARIANTS, build_dir, timing, callgrind))
    measured.update(report("Runtime broker configuration, one option at a time (docs/DESIGN.md)", AXES_VARIANTS,
                           build_dir, timing, callgrind))
    if args.budgets:
        with open(args.budgets) as file:
            budgets = json.load(file)
        errors = check_budgets(measured, budgets)
        print(f"**Regression gate:** {len(budgets['measurements'])} instruction-count limits; "
              + (f"{len(errors)} failure(s)." if errors else "all within budget."))
        for error in errors:
            print(f"- {error}")
        if errors:
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
