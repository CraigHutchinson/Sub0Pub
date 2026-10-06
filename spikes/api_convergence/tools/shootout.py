#!/usr/bin/env python3
"""A/B shootout for the API-convergence spikes (see ../WORKFLOW.md).

This is the project's collapse-evidence tool (tests/collapse/collapse_evidence.py) pointed at the spike cases in
../cases, so every candidate API is judged by the method docs/EVIDENCE.md already defines: a final-link executable
per variant, compared with its equal-work reference on behaviour, instructions per publication, the static publish
path, indirect calls, image, RAM, retained library code and link dependencies.

A variant names its reference in its first lines (`// COLLAPSE_REFERENCE: <variant>`), `handwritten` otherwise:

  static builds of a candidate    against hand-written direct calls (the floor today's StaticWiring reaches)
  dynamic builds of a candidate   against `today_dynamic`, the current Subscribe/Publish API (must not cost more)

What this wrapper adds to the stock tool:

  * the spike driver (../harness/driver.cpp), whose `--instr` mode counts executed instructions by single-stepping
    on Windows x64, so MSVC-ABI builds get the instruction counts that callgrind provides on Linux;
  * `clangcl-O2` and `icx-O2` builds (clang-cl and Intel icx-cl through the MSVC final-image path);
  * `--timing`: wall-clock ns per publication (supplemental, noisy);
  * `--keep DIR`: the measured executables and PDBs, for a profiler (tools/vtune_ab.py);
  * a cross-build summary table of every variant against its reference.

Usage:
  python tools/shootout.py [--case NAME] [--build NAME ...] [--timing] [--keep DIR] [--json OUT.json] > report.md
Exit status is the stock tool's: non-zero on a behaviour mismatch or a build, run or measurement failure.
"""
import argparse
import contextlib
import io
import os
import re
import shutil
import sys
from collections import OrderedDict

from spike_common import collapse_evidence, first_line

ce = collapse_evidence()

BENCH_PUBLISHES = 5_000_000

options = argparse.Namespace(timing=False, keep=None)
measured = OrderedDict()  # (build, case, form, variant) -> result


def compiler_version(build_cfg, stock=ce.compiler_version):
    command = build_cfg.get("version_command")
    return first_line(command) if command else stock(build_cfg)


def single_step(exe):
    """Instructions per phase from the driver's --instr mode, or None where it is unavailable."""
    result = ce.run([exe, "--instr"])
    found = re.search(r"instr setup=(\d+) publish=(\d+) publishes=(\d+) teardown=(\d+)", result.stdout)
    if result.returncode != 0 or not found:
        return None
    setup, publish, publishes, teardown = (int(group) for group in found.groups())
    return {"setup": setup, "publish": publish / publishes, "teardown": teardown}


def bench(exe):
    result = ce.run([exe, "--bench", str(BENCH_PUBLISHES)])
    found = re.search(r"bench ns_per_publish=([0-9.]+)", result.stdout)
    return float(found.group(1)) if result.returncode == 0 and found else None


def measure(build_name, build_cfg, case, variant, form, observable, out_dir, stock=ce.measure):
    result = stock(build_name, build_cfg, case, variant, form, observable, out_dir)
    if "error" in result or not build_cfg["run"]:
        return result
    exe = result["exe"]
    if "instr" not in result and sys.platform == "win32":
        counts = single_step(exe)
        if counts is None:
            return {"error": "the single-step instruction count failed (driver --instr)"}
        result["instr"] = counts
    if options.timing:
        result["bench_ns"] = bench(exe)
    if options.keep:
        target = os.path.join(options.keep, build_name)
        os.makedirs(target, exist_ok=True)
        # Original names (<case>-<variant>-<observable>): the image records its PDB by file name
        for path in (exe, exe[:-4] + ".pdb"):
            if os.path.exists(path):
                shutil.copyfile(path, os.path.join(target, os.path.basename(path)))
    measured[(build_name, case, form, variant)] = result
    return result


def summary():
    """One row per variant, one column per build: observable-work deltas against the variant's reference."""
    builds = list(OrderedDict.fromkeys(key[0] for key in measured))
    cases = list(OrderedDict.fromkeys(key[1] for key in measured))
    lines = ["## Summary: every variant against its reference (observable work)\n",
             "Each cell: instructions per publication (delta), then the deltas of the static publish path, indirect "
             "calls on it, RAM and image text. `=` marks a variant that meets every criterion of docs/EVIDENCE.md "
             "against that reference; `!` one that does not.\n"]
    for case in cases:
        variants = list(OrderedDict.fromkeys(key[3] for key in measured if key[1] == case))
        lines.append(f"### {case}\n")
        lines.append("| variant | judged against | " + " | ".join(builds) + " |")
        lines.append("|---|---|" + "---|" * len(builds))
        for variant in variants:
            reference = ce.variant_reference(case, variant)
            cells = []
            for build in builds:
                result = measured.get((build, case, "observable", variant))
                if result is None:
                    cells.append("-")
                    continue
                instr = result.get("instr", {}).get("publish")
                base = measured.get((build, case, "observable", reference)) if reference else None
                if base is None:
                    cells.append(f"{instr:.1f} instr; path {result['path']['instructions']}" if instr is not None
                                 else f"path {result['path']['instructions']}")
                    continue
                delta = ce.deltas(result, base)
                verdict, _ = ce.verdicts(result, base)
                mark = "=" if all(ok is not False for ok in verdict.values()) else "!"
                head = f"{instr:.1f} ({delta['publish']:+.1f})" if instr is not None and "publish" in delta else "n/a"
                cells.append(f"{mark} {head}; path {delta['path']:+d}; indirect {delta['indirect_calls']:+d}; "
                             f"RAM {delta['ram']:+d}; text {delta['text']:+d}")
            lines.append(f"| {variant} | {reference or 'reference'} | " + " | ".join(cells) + " |")
        lines.append("")
    if options.timing:
        lines.append("## Wall-clock time (supplemental)\n")
        lines.append(f"Best of 9 epochs of {BENCH_PUBLISHES:,} publications, ns per publication. Machine- and "
                     "noise-dependent: the instruction counts above are the comparison of record.\n")
        for case in cases:
            variants = list(OrderedDict.fromkeys(key[3] for key in measured if key[1] == case))
            lines.append(f"### {case}\n")
            lines.append("| variant | " + " | ".join(builds) + " |")
            lines.append("|---|" + "---|" * len(builds))
            for variant in variants:
                cells = []
                for build in builds:
                    value = measured.get((build, case, "observable", variant), {}).get("bench_ns")
                    cells.append(f"{value:.2f}" if value is not None else "-")
                lines.append(f"| {variant} | " + " | ".join(cells) + " |")
            lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--timing", action="store_true")
    parser.add_argument("--keep")
    own, rest = parser.parse_known_args()
    options.timing, options.keep = own.timing, own.keep
    sys.argv = [sys.argv[0], *rest]

    ce.measure = measure
    ce.compiler_version = compiler_version
    report = io.StringIO()
    with contextlib.redirect_stdout(report):
        status = ce.main()
    text = report.getvalue().replace("# Collapse evidence", "# API convergence shootout", 1)
    note = ("\nMeasured with `spikes/api_convergence/tools/shootout.py`: the collapse-evidence method of "
            "docs/EVIDENCE.md applied to the spike cases. On Windows the `instr` columns are exact executed-"
            "instruction counts from the spike driver's single-step mode (`--instr`), not callgrind; the counted "
            "region is the same (the driver's loop, the call and the publication).\n")
    head, separator, tail = text.partition("\n")
    sys.stdout.write(head + separator + note + tail)
    if measured:
        sys.stdout.write("\n" + summary())
    return status


if __name__ == "__main__":
    sys.exit(main())
