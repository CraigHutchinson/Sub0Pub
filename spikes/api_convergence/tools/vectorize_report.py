#!/usr/bin/env python3
"""Vectorisation and aliasing evidence for a burst of publications (see ../WORKFLOW.md).

A delivery that collapses to direct calls leaves the publisher's loop for the compiler to optimise as its own code:
the burst in ../cases/batch is then a reduction loop that can be vectorised. This tool asks each compiler what it did
with that loop (the line marked `// BATCH_LOOP`) in every variant, and why:

  clang   optimisation records (-fsave-optimization-record): whether loop-vectorize vectorised the loop, and how
          many loads on the publish path the optimiser could not hoist or eliminate because a store may alias them
          (licm `LoadWithLoopInvariantAddressInvalidated`, gvn `LoadClobbered`): the aliasing evidence
  MSVC    /Qvec-report:2 (C5001 vectorised; C5002 with its reason code, 1200 being a loop-carried dependence,
          which is how unresolved aliasing shows up)
  icx     /Qopt-report (loop and SLP vectorisation remarks)

Each compiler is asked twice: with its default instruction set, and with AVX2 enabled. The compile line is the
shootout's (the MSVC-ABI builds of tools/shootout.py), so both stages describe the same code; like those builds,
this tool runs on Windows.

A candidate's static and dynamic builds are gated: for every compiler and instruction set the burst loop must be
vectorised whenever its reference's is, and clang must report no more alias-blocked loads than for the reference.
The references are those of tools/shootout.py. Other variants are reported against their reference, not gated.

Usage: python tools/vectorize_report.py [--case batch] > vectorize.md
Exit status is non-zero if a variant fails to compile, has no marked loop, or a gated variant fails the criterion.
"""
import argparse
import os
import re
import shutil
import sys
import tempfile
from collections import OrderedDict

from spike_common import CASES, collapse_evidence, first_line

ce = collapse_evidence()

MARKER = "// BATCH_LOOP"
ROOT_FUNCTION = "collapse_publish"
ALIAS_REMARKS = {("licm", "LoadWithLoopInvariantAddressInvalidated"), ("gvn", "LoadClobbered")}
ARCHES = OrderedDict([("default", []), ("avx2", ["/arch:AVX2"])])


def compile_command(compiler, flags, source, work, extra):
    """The shootout's compile line for a build (tools/shootout.py), compile-only, plus a report option: the two
    stages must describe the same code."""
    return [compiler, *ce.MSVC_COMMON, *ce.BUILDS["msvc-O2"]["flags"], "/DCOLLAPSE_OBSERVABLE=1", "/c", *flags, *extra,
            "/I" + ce.HERE, "/I" + ce.INCLUDE, source, "/Fo" + os.path.join(work, compiler + ".obj"),
            "/Fd" + os.path.join(work, compiler + ".pdb")]


def marked_loop(case, variant):
    """(file name, line) of the loop marked BATCH_LOOP in the variant's source or a header beside it."""
    directory = os.path.join(CASES, case)
    pending, seen = [variant + ".cpp"], set()
    while pending:
        name = pending.pop()
        if name in seen or not os.path.isfile(os.path.join(directory, name)):
            continue
        seen.add(name)
        with open(os.path.join(directory, name), encoding="utf-8") as handle:
            for number, line in enumerate(handle, 1):
                if MARKER in line:
                    return name, number
                included = re.match(r'\s*#\s*include\s+"([^"/]+)"', line)
                if included:
                    pending.append(included.group(1))
    return None


def pick(reports, loop):
    """The report for the marked loop: the copy inlined into the publication entry point if there is one."""
    here = [r for r in reports if (r["file"].lower(), r["line"]) == (loop[0].lower(), loop[1])]
    rooted = [r for r in here if r["function"] == ROOT_FUNCTION]
    return (rooted or here or [None])[0]


def clang(source, flags, work, loop):
    record = os.path.join(work, "remarks.yaml")
    command = compile_command("clang-cl", flags, source, work,
                              ["/clang:-fsave-optimization-record", "/clang:-foptimization-record-file=" + record])
    result = ce.run(command)
    if result.returncode != 0 or not os.path.isfile(record):
        return {"error": next((l for l in (result.stdout + result.stderr).splitlines() if "error" in l), "clang-cl failed")}
    reports, alias = [], 0
    with open(record, encoding="utf-8", errors="replace") as handle:
        documents = re.split(r"^--- !", handle.read(), flags=re.M)[1:]
    for document in documents:
        kind = document.split("\n", 1)[0].strip()
        field = {key: re.search(rf"^{key}:\s*(.*)$", document, re.M) for key in ("Pass", "Name", "Function")}
        where = re.search(r"DebugLoc:\s*\{\s*File:\s*'?([^,']*)'?,\s*Line:\s*(\d+)", document)
        if not all(field.values()):
            continue
        pass_name, name, function = (field[key].group(1).strip().strip("'") for key in ("Pass", "Name", "Function"))
        if function == ROOT_FUNCTION and (pass_name, name) in ALIAS_REMARKS and kind == "Missed":
            alias += 1
        if pass_name != "loop-vectorize" or not where:
            continue
        entry = {"function": function, "file": os.path.basename(where.group(1).replace("\\", "/")),
                 "line": int(where.group(2)), "vectorized": kind == "Passed" and name == "Vectorized", "detail": name,
                 "kind": kind}
        if entry["vectorized"]:
            width = re.search(r"VectorizationFactor:\s*'?(\d+)", document)
            count = re.search(r"InterleaveCount:\s*'?(\d+)", document)
            entry["detail"] = f"width {width.group(1) if width else '?'} x {count.group(1) if count else '?'}"
        reports.append(entry)
    # A loop has several records (analysis, then the outcome): the outcome decides
    here = [r for r in reports if (r["file"].lower(), r["line"]) == (loop[0].lower(), loop[1])]
    rooted = [r for r in here if r["function"] == ROOT_FUNCTION] or here
    done = next((r for r in rooted if r["vectorized"]), None)
    if done:
        return {"vectorized": True, "detail": done["detail"], "alias": alias}
    # The vectoriser states its reason where the offending access is (often inside the inlined receive()), not on
    # the loop line: take the analysis records of the whole entry point
    reasons = sorted({r["detail"] for r in reports if r["function"] == ROOT_FUNCTION and r["kind"] == "Analysis"})
    if not reasons:
        reasons = ["loop-vectorize gave up" if rooted else "not a candidate: the loop body is opaque to the vectoriser"]
    return {"vectorized": False, "detail": ", ".join(reasons), "alias": alias}


def msvc(source, flags, work, loop):
    result = ce.run(compile_command("cl", flags, source, work, ["/Qvec-report:2"]))
    if result.returncode != 0:
        return {"error": next((l for l in result.stdout.splitlines() if " error " in l), "cl failed")}
    reports, function = [], ""
    for line in result.stdout.splitlines():
        analysing = re.match(r"--- Analyzing function: (.*)", line)
        if analysing:
            function = analysing.group(1).strip()
            continue
        info = re.match(r"(.*)\((\d+)\) : info C(5001|5002): (.*)", line)
        if info:
            reason = re.search(r"reason '(\d+)'", info.group(4))
            reports.append({"function": function, "file": os.path.basename(info.group(1).replace("\\", "/")),
                            "line": int(info.group(2)), "vectorized": info.group(3) == "5001",
                            "detail": "C5001" if info.group(3) == "5001" else f"C5002 reason {reason.group(1) if reason else '?'}"})
    found = pick(reports, loop)
    if found is None:
        return {"vectorized": False, "detail": "no vectoriser message for the loop"}
    return {"vectorized": found["vectorized"], "detail": found["detail"]}


def icx(source, flags, work, loop):
    report = os.path.join(work, "icx.optrpt")
    result = ce.run(compile_command("icx-cl", flags, source, work, ["/Qopt-report:2", "/Qopt-report-file:" + report]))
    if result.returncode != 0 or not os.path.isfile(report):
        return {"error": next((l for l in (result.stdout + result.stderr).splitlines() if "error" in l), "icx-cl failed")}
    reports, function, current = [], "", None
    with open(report, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            begin = re.match(r"Begin optimization report for: (.*)", line)
            if begin:
                function = begin.group(1).strip()
                continue
            loop_begin = re.match(r"\s*LOOP BEGIN at (.*) \((\d+), \d+\)", line)
            if loop_begin:
                current = {"function": function, "file": os.path.basename(loop_begin.group(1).replace("\\", "/")),
                           "line": int(loop_begin.group(2)), "vectorized": False, "detail": ""}
                reports.append(current)
                continue
            if current is not None and "LOOP END" in line:
                current = None
                continue
            if current is None:
                continue
            if "LOOP WAS VECTORIZED" in line.upper():
                current["vectorized"], current["detail"] = True, "loop vectorised"
            elif "SLP vectorization" in line and not current["vectorized"]:
                current["vectorized"], current["detail"] = True, "SLP vectorised after unrolling"
            elif "not vectorized" in line and not current["vectorized"]:
                current["detail"] = line.split("not vectorized:", 1)[-1].strip().rstrip(".")[:60]
    found = pick(reports, loop)
    if found is None:
        return {"vectorized": False, "detail": "no loop report for the loop"}
    return {"vectorized": found["vectorized"], "detail": found["detail"] or "no vectorisation remark"}


COMPILERS = OrderedDict([("clang", ("clang-cl", clang, lambda: first_line(["clang-cl", "--version"]))),
                         ("msvc", ("cl", msvc, lambda: ce.compiler_version(ce.BUILDS["msvc-O2"]))),
                         ("icx", ("icx-cl", icx, lambda: first_line(["icx-cl", "--version"])))])
# The convergence claims that are gated: a candidate's static build against hand-written code and its dynamic build
# against the current API. Other variants (a bridged topology, the current APIs themselves) are reported only.
GATED = re.compile(r"^(?!today_|handwritten).*_(static|dynamic)$")


def cell(result):
    if "error" in result:
        return f"**FAILED** `{result['error'][:80]}`"
    text = ("vectorised" if result["vectorized"] else "not vectorised") + f" ({result['detail']})"
    return text + (f"; alias-blocked loads {result['alias']}" if "alias" in result else "")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--case", default="batch")
    args = parser.parse_args()

    compilers = OrderedDict((name, spec) for name, spec in COMPILERS.items() if shutil.which(spec[0]))
    if not compilers:
        sys.exit("no compiler found (clang-cl, cl or icx-cl)")
    variants = ce.discover_cases(args.case).get(args.case)
    if not variants:
        sys.exit(f"no such case: {args.case}")

    results, failures = OrderedDict(), []
    with tempfile.TemporaryDirectory() as work:
        for variant in variants:
            loop = marked_loop(args.case, variant)
            if loop is None:
                failures.append(f"{variant}: no loop marked {MARKER}")
                continue
            source = os.path.join(CASES, args.case, variant + ".cpp")
            for compiler, (_, analyse, _) in compilers.items():
                for arch, flags in ARCHES.items():
                    results[(variant, compiler, arch)] = result = analyse(source, flags, work, loop)
                    if "error" in result:
                        failures.append(f"{variant}/{compiler}/{arch}: {result['error']}")

    columns = [(compiler, arch) for compiler in compilers for arch in ARCHES]
    print(f"# Vectorisation and aliasing: case `{args.case}`\n")
    print("What each compiler did with the burst loop (the line marked `// BATCH_LOOP`) in every variant, compiled as "
          "the shootout compiles it (`tools/vectorize_report.py`). For clang, `alias-blocked loads` counts the loads on the publish path that "
          "could not be hoisted or eliminated because a store may alias them. MSVC `C5002 reason 1200` is a "
          "loop-carried dependence, which is how unresolved aliasing shows there; `reason 1102` is an operation its "
          "selected instruction set cannot vectorise.\n")
    for compiler, (_, _, version) in compilers.items():
        print(f"- **{compiler}**: `{version()}`")
    print()
    print("| variant | judged against | " + " | ".join(f"{c} {a}" for c, a in columns) + " | verdict |")
    print("|---|---|" + "---|" * (len(columns) + 1))
    for variant in variants:
        if (variant, columns[0][0], columns[0][1]) not in results:
            continue
        reference = ce.variant_reference(args.case, variant)
        problems = []
        for compiler, arch in columns:
            mine = results[(variant, compiler, arch)]
            base = results.get((reference, compiler, arch)) if reference else None
            if base is None or "error" in mine or "error" in base:
                continue
            if base["vectorized"] and not mine["vectorized"]:
                problems.append(f"{compiler} {arch}: lost vectorisation")
            if mine.get("alias", 0) > base.get("alias", 0):
                problems.append(f"{compiler} {arch}: alias-blocked loads {base.get('alias', 0)} -> {mine['alias']}")
        gated = bool(GATED.match(variant))
        if reference is None:
            verdict = "reference"
        elif not problems:
            verdict = "PASS" if gated else "same as its reference"
        else:
            verdict = ("FAIL: " if gated else "reported, not gated: ") + "; ".join(problems)
        print(f"| {variant} | {reference or '-'} | " + " | ".join(cell(results[(variant, c, a)]) for c, a in columns)
              + f" | {verdict} |")
        if problems and gated:
            failures.append(f"{variant}: " + "; ".join(problems))
    print()
    if failures:
        print("**Failed:** " + "; ".join(failures) + "\n")
        print(f"{len(failures)} failure(s)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
