#!/usr/bin/env python3
"""v1 vs v2 evidence for MIGRATION.md: the same scenarios against every implementation on the table.

Implementations:
  * v1.0          the v1.0 tag's include/sub0pub/ (read with `git archive`), default and ThreadSafe
  * v2 header     the current include/sub0pub/ (umbrella sub0pub.hpp), each dispatch policy
  * v2 config     per-type configurations of the current header (Section 2), selected options
  * v2 wiring     static wiring of the current header (Section 4): StaticWiring, wire(), Sink<T>, DynamicPort
  * hand-written  direct calls and a virtual loop, the floor

Metrics:
  * instr/op  exact instructions per operation under callgrind, per host compiler (-O2). Includes about
              3 instructions of loop overhead. This is the regression bar; wall-clock time is not reported.
  * footprint object text/data/bss and link-time dependencies of one publisher, one subscriber and one
              publish site, compiled -Os -fno-exceptions -fno-rtti as tests/footprint/measure_footprint.py.

Usage (Linux, valgrind; arm-none-eabi-g++ optional; run from anywhere inside the repository):
  python3 tests/compare/compare_versions.py [--v1-ref v1.0] > report.md
"""
import argparse
import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
TESTS = os.path.join(ROOT, "tests")

spec = importlib.util.spec_from_file_location("run_baseline", os.path.join(TESTS, "bench", "run_baseline.py"))
run_baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(run_baseline)

SCENARIOS = [
    "publish, 0 subscribers",
    "publish, 1 subscriber",
    "publish, 8 subscribers",
    "publish, 1 filtered subscriber (pass)",
    "publish, 8 subscribers, first cancels",
    "create + destroy subscriber",
]
SHORT = ["publish 0", "publish 1", "publish 8", "filtered 1", "8, first cancels", "create + destroy"]

V2_OFF = ["-DSUB0PUB_REENTRANT_SAFE=false"]
V2_FULL = ["-DSUB0PUB_REENTRANT_SAFE=true", "-DSUB0PUB_CANCEL=true", "-DSUB0PUB_FILTER=true"]
# (label, header, macros): cmp_sub0pub.cpp built once per entry
HEADER_VARIANTS = [
    ("v1.0", "v1", []),
    ("v1.0 ThreadSafe (mutex)", "v1", ["-DSUB0PUB_THREAD_SAFE=true"]),
    ("v2 default (Direct)", "v2", []),
    ("v2 default + debug checks", "v2", ["-DSUB0PUB_REENTRANT_CHECK=true", "-DSUB0PUB_THREAD_CHECK=true"]),
    ("v2 Full (snapshot, cancel, filter)", "v2", V2_FULL),
    ("v2 ThreadSafe (mutex + snapshot, filter)", "v2", ["-DSUB0PUB_THREAD_SAFE=true", "-DSUB0PUB_FILTER=true"]),
]
HOST_COMPILERS = OrderedDict([("gcc", "g++"), ("clang", "clang++")])
HOST_FLAGS = ["-std=c++23", "-O2", "-DNDEBUG", "-pthread"]

FP_COMMON = ["-std=c++23", "-Os", "-DNDEBUG", "-fno-exceptions", "-fno-rtti", "-ffunction-sections", "-fdata-sections"]
FP_TARGETS = OrderedDict([
    ("cm33", {"cxx": "arm-none-eabi-g++", "size": "arm-none-eabi-size", "nm": "arm-none-eabi-nm",
              "flags": ["-mcpu=cortex-m33", "-mthumb", "-mfloat-abi=hard", "-mfpu=fpv5-sp-d16"]}),
    ("host", {"cxx": "g++", "size": "size", "nm": "nm", "flags": []}),
])
# (label, source, include set, macros)
FP_VARIANTS = [
    ("v1.0", "footprint/fp_1type.cpp", "v1", ["-include", "stdexcept"]),
    ("v2 default (Direct)", "footprint/fp_1type.cpp", "v2", []),
    ("v2 Full (snapshot, cancel, filter)", "footprint/fp_1type.cpp", "v2", V2_FULL),
    ("v2 config Lean", "compare/fp_config_lean.cpp", "v2", []),
    ("v2 StaticWiring", "compare/fp_static.cpp", "v2", []),
]
# Link-time dependencies worth naming: anything else is the translation unit's own code
DEPENDENCY_HINTS = ("__tls", "__aeabi_read_tp", "tls", "mutex", "pthread", "memcpy", "memmove", "operator delete",
                    "__cxa", "abort", "__stack_chk")


def git(*args):
    return subprocess.run(["git", "-C", ROOT, *args], capture_output=True, text=True, check=True).stdout


def header_at(tmp, name, ref):
    """Write the library headers at a git ref into their own include directory: the single sub0pub.hpp of v1.0,
    or the split include/sub0pub/ tree (an umbrella sub0pub.hpp and the headers it includes) of later revisions"""
    base = os.path.join(tmp, name)
    os.makedirs(base)
    archive = subprocess.run(["git", "-C", ROOT, "archive", "--format=tar", ref, "include/sub0pub"],
                             capture_output=True, check=True).stdout
    subprocess.run(["tar", "-x", "-C", base], input=archive, check=True)
    return os.path.join(base, "include")


def include_sets(tmp, v1_ref, extra_refs):
    common = [os.path.join(TESTS, d) for d in ("vendor", "bench", "compare", "collapse")]
    sets = {"v1": [header_at(tmp, "v1", v1_ref)] + common, "v2": [os.path.join(ROOT, "include")] + common}
    for i, (_, ref) in enumerate(extra_refs):
        sets[f"extra{i}"] = [header_at(tmp, f"extra{i}", ref)] + common
    return sets


def build(cxx, src, includes, macros, exe, flags):
    cmd = [cxx, *flags, *(f"-I{i}" for i in includes), *macros, src, "-o", exe]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"build failed: {' '.join(cmd)}\n{r.stderr[-2000:]}")
    return exe


def measure(compiler, tmp, incs, extra_refs):
    """Return OrderedDict {row label: {scenario: instr/op}} for one host compiler."""
    rows = OrderedDict()

    def collect(exe):
        measurements = run_baseline.run_callgrind(exe)
        if not measurements:
            sys.exit(f"no instruction measurements from {exe}; install Valgrind headers and rebuild")
        for (section, scenario), value in measurements.items():
            rows.setdefault(section, {})[scenario] = value

    variants = list(HEADER_VARIANTS)
    for i, (label, _) in enumerate(extra_refs):
        variants += [(f"{label} Snapshot (default)", f"extra{i}", []),
                     (f"{label} Direct unchecked", f"extra{i}", V2_OFF + ["-DSUB0PUB_REENTRANT_CHECK=false"])]
    for i, (label, header, macros) in enumerate(variants):
        exe = build(compiler, os.path.join(HERE, "cmp_sub0pub.cpp"), incs[header],
                    macros + [f'-DCMP_LABEL="{label}"'], os.path.join(tmp, f"{compiler}_hdr{i}"), HOST_FLAGS)
        collect(exe)
    for name in ("cmp_config", "cmp_static", "cmp_mixed"):
        collect(build(compiler, os.path.join(HERE, name + ".cpp"), incs["v2"], [],
                      os.path.join(tmp, f"{compiler}_{name}"), HOST_FLAGS))
    return rows


def footprint(target, tmp, incs):
    results = OrderedDict()
    for i, (label, src, inc, macros) in enumerate(FP_VARIANTS):
        obj = os.path.join(tmp, f"fp_{target['cxx']}_{i}.o")
        cmd = [target["cxx"], *FP_COMMON, *target["flags"], *(f"-I{p}" for p in incs[inc]), *macros,
               "-c", os.path.join(TESTS, src), "-o", obj]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            err = next((l for l in r.stderr.splitlines() if "error" in l), r.stderr.strip()[:120])
            results[label] = {"error": err.strip()}
            continue
        size = subprocess.run([target["size"], obj], capture_output=True, text=True, check=True).stdout.splitlines()[1]
        undef = subprocess.run([target["nm"], "-C", "-u", obj], capture_output=True, text=True, check=True).stdout
        deps = sorted({l.split(None, 1)[-1].strip() for l in undef.splitlines() if l.strip()})
        results[label] = {"size": tuple(int(x) for x in size.split()[:3]),
                          "deps": [d for d in deps if any(h in d for h in DEPENDENCY_HINTS)]}
    return results


def fmt(value):
    return "n/a" if value is None else f"{value:.1f}"


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--v1-ref", default="v1.0", help="git ref holding the v1 header (default: v1.0)")
    parser.add_argument("--extra-ref", action="append", default=[], metavar="LABEL=REF",
                        help="also measure the header at a git ref (default and Direct unchecked), e.g. \"v2 before=9b9d759\"")
    args = parser.parse_args()
    extra_refs = [tuple(e.split("=", 1)) for e in args.extra_ref]
    if not shutil.which("valgrind"):
        sys.exit("valgrind is required (instruction counts are the comparison metric)")

    v1_sha = git("rev-parse", "--short", f"{args.v1_ref}^{{commit}}").strip()
    head_sha = git("rev-parse", "--short", "HEAD").strip()
    print("# v1 vs v2: measured comparison\n")
    extras = "".join(f"; {label}: `{ref}`" for label, ref in extra_refs)
    print(f"Generated by `tests/compare/compare_versions.py`. v1 header: `{args.v1_ref}` (`{v1_sha}`){extras}; "
          f"everything else: `{head_sha}`.\n")

    with tempfile.TemporaryDirectory() as tmp:
        incs = include_sets(tmp, args.v1_ref, extra_refs)
        for name, cxx in HOST_COMPILERS.items():
            if not shutil.which(cxx):
                continue
            version = subprocess.run([cxx, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
            rows = measure(cxx, tmp, incs, extra_refs)
            print(f"## instr/op, {name} (`{version}`, `{' '.join(HOST_FLAGS)}`)\n")
            def table(selected, scenarios, headings):
                print("| Implementation | " + " | ".join(headings) + " |")
                print("|---|" + "---:|" * len(headings))
                for label, values in selected:
                    print(f"| {label} | " + " | ".join(fmt(values.get(s)) for s in scenarios) + " |")
                print()

            table([(k, v) for k, v in rows.items() if not k.startswith("v2 mixed")], SCENARIOS, SHORT)
            print("### Mixed paths (v2 only)\n")
            print("Every publish also delivers to one fixed receiver. Counts below name only dynamic listeners; "
                  "do not compare these rows to the pure-dispatch table as identical work. Churn creates and "
                  "destroys a second listener while one remains registered; overflow attempts registration "
                  "with all eight slots occupied.\n")
            table([(k, v) for k, v in rows.items() if k.startswith("v2 mixed")],
                  [SCENARIOS[i] for i in (0, 1, 2, 5)] + ["registration rejected at capacity"],
                  ["fixed + 0", "fixed + 1", "fixed + 8", "churn beside 1", "full registration"])

        for tname, target in FP_TARGETS.items():
            if not shutil.which(target["cxx"]):
                continue
            print(f"## Footprint, {tname} (`{' '.join(FP_COMMON[:5] + target['flags'])}`)\n")
            print("One publisher, one subscriber, one publish site; object file sizes in bytes.\n")
            print("| Implementation | text | data | bss | Link-time dependencies |")
            print("|---|---:|---:|---:|---|")
            for label, r in footprint(target, tmp, incs).items():
                if "error" in r:
                    print(f"| {label} | n/a | | | does not compile: `{r['error']}` |")
                else:
                    deps = ", ".join(f"`{d}`" for d in r["deps"]) or "none"
                    print(f"| {label} | {r['size'][0]} | {r['size'][1]} | {r['size'][2]} | {deps} |")
            print()


if __name__ == "__main__":
    main()
