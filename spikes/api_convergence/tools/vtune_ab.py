#!/usr/bin/env python3
"""VTune A/B of shootout variants: where the publication's CPU time goes (see ../WORKFLOW.md).

Runs Intel VTune Profiler's hotspots collection on the shootout executables of one case (tools/shootout.py --keep),
each driven by its `--bench` mode for a few seconds, and tabulates per variant:

  * ns per publication measured under the profiler (wall clock, noisy: supplemental);
  * CPU time attributed to the executable, and the share of it spent in Sub0Pub functions (`sub0::...`). A delivery
    that collapsed has no such frames; the runtime broker's share is the dispatch machinery a profile will show;
  * the functions holding most of the time.

Sampling mode `sw` (user-mode sampling) needs no privileges and resolves functions through the kept PDBs. Mode `hw`
(hardware event-based sampling) needs the VTune sampling driver and an elevated prompt; it is the route to the
microarchitecture metrics (indirect-branch mispredictions, front-end stalls) that instruction counts cannot show.

Usage:
  python tools/vtune_ab.py --keep DIR [--build msvc-O2] [--case station] [--variant NAME ...] [--mode sw|hw] > vtune.md
The executables are built into DIR by tools/shootout.py first if they are not there yet.
"""
import argparse
import csv
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import OrderedDict

from spike_common import SPIKE, collapse_evidence, first_line

ce = collapse_evidence()

DEFAULT_VARIANTS = ["handwritten", "today_static", "route_static", "today_dynamic", "route_dynamic", "bus_dynamic"]
EPOCHS = 9  # the driver's --bench runs this many timed epochs
CALIBRATION_PUBLISHES = 1_000_000


def bench(command):
    result = ce.run(command)
    found = re.search(r"bench ns_per_publish=([0-9.]+)", result.stdout + result.stderr)
    return float(found.group(1)) if found else None


def profile(vtune, exe, publishes, mode, work):
    """(ns per publication under the profiler, {function: CPU seconds} within the executable), or an error string."""
    result_dir = os.path.join(work, "result")
    shutil.rmtree(result_dir, ignore_errors=True)
    collect = ce.run([vtune, "-collect", "hotspots", "-knob", "sampling-mode=" + mode, "-result-dir", result_dir,
                      "-quiet", "--", exe, "--bench", str(publishes)])
    output = collect.stdout + collect.stderr
    if collect.returncode != 0 or not os.path.isdir(result_dir):
        return next((line.strip() for line in output.splitlines() if "Error" in line), "vtune collection failed")
    found = re.search(r"bench ns_per_publish=([0-9.]+)", output)
    report = ce.run([vtune, "-report", "hotspots", "-r", result_dir, "-group-by", "function", "-format", "csv",
                     "-csv-delimiter", "comma", "-quiet"])
    if report.returncode != 0:
        return "vtune report failed"
    module = os.path.basename(exe).lower()
    functions = OrderedDict()
    for row in csv.DictReader(io.StringIO(report.stdout)):
        if (row.get("Module") or "").lower() != module:
            continue
        name = row.get("Function (Full)") or row.get("Function") or "?"
        functions[name] = functions.get(name, 0.0) + float(row.get("CPU Time") or 0.0)
    return (float(found.group(1)) if found else None), functions


def short(name):
    """A function name without its parameter list or the anonymous-namespace noise."""
    name = re.sub(r"`anonymous[ -]namespace'::", "", name)
    return re.sub(r"\(.*\)$", "", name)[:70]


def is_library(name):
    """Whether a function belongs to Sub0Pub: its own qualified name starts with `sub0::`. An application function
    that merely names a Sub0Pub type in its template arguments (`Sensor<sub0::StaticWiring<...>>::send`) does not."""
    return short(name).startswith("sub0::")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--keep", required=True, help="directory of kept shootout executables (tools/shootout.py --keep)")
    parser.add_argument("--build", default="msvc-O2")
    parser.add_argument("--case", default="station")
    parser.add_argument("--variant", action="append", help="variant to profile (repeat); default: the A/B set")
    parser.add_argument("--mode", choices=("sw", "hw"), default="sw")
    parser.add_argument("--seconds", type=float, default=3.0, help="target run time of each profiled workload")
    args = parser.parse_args()

    vtune = shutil.which("vtune")
    if not vtune:
        sys.exit("vtune not found: install Intel VTune Profiler or add its bin64 directory to PATH")
    variants = args.variant or DEFAULT_VARIANTS
    directory = os.path.join(args.keep, args.build)

    def executable(variant):
        return os.path.join(directory, f"{args.case}-{variant}-1.exe")

    if not all(os.path.isfile(executable(v)) for v in variants):
        built = subprocess.run([sys.executable, os.path.join(SPIKE, "tools", "shootout.py"), "--case", args.case,
                                "--build", args.build, "--keep", args.keep], capture_output=True, text=True)
        missing = [v for v in variants if not os.path.isfile(executable(v))]
        if missing:
            sys.exit(f"no executable for {', '.join(missing)} (shootout exit {built.returncode})")

    print(f"# VTune A/B: case `{args.case}`, build `{args.build}`\n")
    print(f"`{first_line([vtune, '--version'])}`, hotspots collection, sampling mode `{args.mode}` "
          f"(`tools/vtune_ab.py`). Each variant publishes in a loop for about {args.seconds:g} s. `Sub0Pub share` is the "
          "part of the executable's CPU time that VTune attributes to functions of namespace `sub0`; application code "
          "inlined into such a function counts towards it. A collapsed delivery has no such function left to sample.\n")
    print("| variant | ns / publication | CPU time in image (s) | Sub0Pub share | where the time is |")
    print("|---|---:|---:|---:|---|")
    failures = []
    with tempfile.TemporaryDirectory() as work:
        for variant in variants:
            exe = executable(variant)
            calibration = bench([exe, "--bench", str(CALIBRATION_PUBLISHES)])
            if calibration is None:
                failures.append(f"{variant}: the benchmark mode did not run")
                continue
            publishes = max(CALIBRATION_PUBLISHES, int(args.seconds * 1e9 / (max(calibration, 0.05) * EPOCHS)))
            outcome = profile(vtune, exe, publishes, args.mode, work)
            if isinstance(outcome, str):
                failures.append(f"{variant}: {outcome}")
                print(f"| {variant} | - | - | - | **FAILED**: {outcome} |")
                continue
            ns, functions = outcome
            total = sum(functions.values())
            library = sum(seconds for name, seconds in functions.items() if is_library(name))
            top = sorted(functions.items(), key=lambda item: -item[1])[:4]
            where = "; ".join(f"`{short(name)}` {100 * seconds / total:.0f}%" for name, seconds in top) if total else "-"
            share = f"{100 * library / total:.0f}%" if total else "-"
            print(f"| {variant} | {ns:.2f} | {total:.2f} | {share} | {where} |" if ns is not None
                  else f"| {variant} | - | {total:.2f} | {share} | {where} |")
            sys.stdout.flush()
    print()
    if failures:
        print("**Failed:** " + "; ".join(failures) + "\n")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
