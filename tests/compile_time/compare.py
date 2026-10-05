#!/usr/bin/env python3
"""Opt-in GCC/Clang compile-time A/B measurement of immutable Sub0Pub header snapshots.

Time serial, clean object compilation of identical consumer translation units. Excludes git extraction,
source generation, warmups, configure and link; includes compiler process startup and code generation.
Uses warm filesystem caches, no PCH/modules/compiler cache. Wall time is advisory, not a noisy CI gate.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).with_name("consumer.cpp")
PROFILES = {"wiring": 0, "broker": 1, "umbrella": 2, "layout": 3}


def command(args, **kwargs):
    return subprocess.run(args, check=True, capture_output=True, **kwargs)


def revision(ref):
    return command(["git", "-C", str(ROOT), "rev-parse", "--verify", "--end-of-options",
                    ref + "^{commit}"], text=True).stdout.strip()


def snapshot(sha, destination):
    archive = command(["git", "-C", str(ROOT), "archive", sha, "include/sub0pub"]).stdout
    # Materialize only regular header files, never links or paths outside the requested include subtree.
    with tarfile.open(fileobj=io.BytesIO(archive)) as tree:
        for member in tree:
            path = Path(member.name)
            if member.isfile() and path.parts[:2] == ("include", "sub0pub") and ".." not in path.parts:
                output = destination / path
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(tree.extractfile(member).read())
    if not (destination / "include/sub0pub/sub0pub.hpp").is_file():
        raise ValueError("revision has no Sub0Pub umbrella header: " + sha)


def summary(values):
    median = statistics.median(values)
    return {"median_s": median, "min_s": min(values), "max_s": max(values),
            "mad_s": statistics.median(abs(value - median) for value in values)}


def lane_order(round_index):
    return ("A", "B") if round_index % 2 == 0 else ("B", "A")


def measure(compiler, flags, include, sources, objects, profile, args, env):
    objects.mkdir()
    try:
        start = time.perf_counter()
        for index, source in enumerate(sources):
            command([compiler, *flags, "-I" + str(include), f"-DPROFILE={profile}",
                     f"-DMESSAGE_TYPES={args.types}", f"-DRECEIVER_COUNT={args.receivers}",
                     f"-DTRANSLATION_UNIT={index}", "-c", str(source), "-o", str(objects / f"{index}.o")],
                    env=env)
        elapsed = time.perf_counter() - start
        # A successful wrapper that produced no objects is not a valid compiler measurement.
        if any(not (objects / f"{index}.o").is_file() for index in range(len(sources))):
            raise ValueError("compiler returned success without producing all object files")
        return elapsed
    finally:
        shutil.rmtree(objects)


def markdown(result):
    metadata = result["metadata"]
    lines = ["# Sub0Pub compile-time A/B", "",
             f"- UTC: {metadata['utc']}",
             f"- Compiler: `{metadata['compiler_version'].splitlines()[0]}`",
             f"- Host: `{metadata['platform']}`; CPU: {metadata['cpu']}; logical CPUs: {metadata['logical_cpus']}",
             f"- A: `{metadata['lanes']['A']['sha']}`, `{metadata['lanes']['A']['standard']}`",
             f"- B: `{metadata['lanes']['B']['sha']}`, `{metadata['lanes']['B']['standard']}`",
             f"- {metadata['translation_units']} TUs/batch, {metadata['types']} message types/TU, "
             f"{metadata['receivers']} receivers; {metadata['samples']} paired samples, "
             f"{metadata['warmups']} warmup batches/lane/profile, serial compilation.",
             "- Flags: `-O2 -DNDEBUG -pthread`; no PCH, modules, compiler cache, configure or link.",
             "- Warm filesystem cache; each sample recompiles every TU to fresh object files.", "",
             "| Workload | A median [min, max] s | B median [min, max] s | B vs A | A / B MAD s |",
             "|---|---:|---:|---:|---:|"]
    for row in result["results"]:
        a, b = row["summary"]["A"], row["summary"]["B"]
        lines.append(f"| {row['profile']} | {a['median_s']:.3f} [{a['min_s']:.3f}, {a['max_s']:.3f}] "
                     f"| {b['median_s']:.3f} [{b['min_s']:.3f}, {b['max_s']:.3f}] "
                     f"| {row['median_change_percent']:+.2f}% | {a['mad_s']:.3f} / {b['mad_s']:.3f} |")
    lines += ["", "Positive change means slower. Samples alternate A/B then B/A; raw paired timings and harness/fixture",
              "hashes are in the JSON. Ranges and MAD describe observed noise, not confidence intervals.",
              "Synthetic serial clean-compile cost is not parallel application build time or incremental/no-op time.",
              "Compare revisions within this run; do not compare absolute seconds across different hosts.", ""]
    return "\n".join(lines)


def run(args):
    compiler = shutil.which(args.compiler)
    if compiler is None:
        raise ValueError("compiler not found: " + args.compiler)
    if Path(compiler).resolve().name in ("ccache", "sccache"):
        raise ValueError("select a direct compiler executable, not a cache wrapper")
    version = command([compiler, "--version"], text=True).stdout.strip()
    lanes = {"A": {"sha": revision(args.baseline), "standard": args.baseline_standard},
             "B": {"sha": revision(args.candidate), "standard": args.candidate_standard}}
    cpu = platform.processor() or "unknown"
    if Path("/proc/cpuinfo").exists():
        cpu = next((line.split(":", 1)[1].strip() for line in Path("/proc/cpuinfo").read_text().splitlines()
                    if line.startswith("model name")), cpu)
    result = {"schema_version": 1, "metadata": {
        "utc": datetime.now(timezone.utc).isoformat(), "compiler": compiler, "compiler_version": version,
        "platform": platform.platform(), "cpu": cpu, "logical_cpus": os.cpu_count(), "lanes": lanes,
        "cpu_affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "include_environment": {key: os.environ[key] for key in ("CPATH", "CPLUS_INCLUDE_PATH", "SDKROOT")
                                if key in os.environ},
        "harness_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "fixture_sha256": hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
        "translation_units": args.translation_units, "types": args.types, "receivers": args.receivers,
        "samples": args.samples, "warmups": args.warmups, "parallelism": 1,
        "cache_policy": "warm filesystem; fresh objects; direct compiler; no PCH/modules",
        "command_template": [compiler, "-std={lane.standard}", "-O2", "-DNDEBUG", "-pthread",
                             "-I{snapshot}/include", "-DPROFILE={profile}", "-DMESSAGE_TYPES={types}",
                             "-DRECEIVER_COUNT={receivers}", "-DTRANSLATION_UNIT={index}",
                             "-c", "{source}", "-o", "{fresh-object}"]}, "results": []}
    env = dict(os.environ, CCACHE_DISABLE="1", SCCACHE_DISABLE="1")
    with tempfile.TemporaryDirectory(prefix="sub0pub-compile-") as directory:
        work = Path(directory)
        for lane, data in lanes.items():
            snapshot(data["sha"], work / lane)
        sources = [work / f"consumer_{index}.cpp" for index in range(args.translation_units)]
        for source in sources:
            source.write_bytes(FIXTURE.read_bytes())
        for profile in args.profiles:
            values = {"A": [], "B": []}
            pairs = []
            for round_index in range(args.warmups + args.samples):
                pair = {"order": list(lane_order(round_index)), "seconds": {}}
                for lane in pair["order"]:
                    print(f"{profile}: {'warmup' if round_index < args.warmups else 'sample'} "
                          f"{round_index + 1}/{args.warmups + args.samples} {lane}", file=sys.stderr, flush=True)
                    elapsed = measure(compiler, [f"-std={lanes[lane]['standard']}", "-O2", "-DNDEBUG", "-pthread"],
                                      work / lane / "include", sources, work / "objects", PROFILES[profile], args, env)
                    if round_index >= args.warmups:
                        values[lane].append(elapsed)
                        pair["seconds"][lane] = elapsed
                if round_index >= args.warmups:
                    pairs.append(pair)
            stats = {lane: summary(times) for lane, times in values.items()}
            result["results"].append({"profile": profile, "samples": pairs, "summary": stats,
                                      "median_change_percent": 100 * (stats['B']['median_s'] / stats['A']['median_s'] - 1)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, help="immutable snapshot resolved from this git ref")
    parser.add_argument("--candidate", default="HEAD", help="git ref; working-tree edits are never measured")
    parser.add_argument("--baseline-standard", choices=["c++17", "c++20", "c++23"], default="c++23")
    parser.add_argument("--candidate-standard", choices=["c++17", "c++20", "c++23"], default="c++23")
    parser.add_argument("--compiler", default="g++", help="direct GCC/Clang executable (no shell flags)")
    parser.add_argument("--profiles", nargs="+", choices=list(PROFILES), default=list(PROFILES))
    parser.add_argument("--translation-units", type=int, default=16)
    parser.add_argument("--types", type=int, default=8)
    parser.add_argument("--receivers", type=int, default=4)
    parser.add_argument("--samples", type=int, default=5)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--json", type=Path, required=True)
    parser.add_argument("--markdown", type=Path, required=True)
    args = parser.parse_args()
    if args.samples < 3 or args.warmups < 1 or args.translation_units < 1 or args.types < 1 or not 1 <= args.receivers <= 8:
        parser.error("require >=3 samples, >=1 warmup/TU/type, and 1..8 receivers (broker capacity)")
    if args.json.resolve() == args.markdown.resolve():
        parser.error("JSON and Markdown paths must differ")
    try:
        result = run(args)
        for path, text in [(args.json, json.dumps(result, indent=2) + "\n"), (args.markdown, markdown(result))]:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"compile benchmark failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError) and error.stderr:
            print(error.stderr.decode(errors="replace") if isinstance(error.stderr, bytes) else error.stderr, file=sys.stderr)
        return 1
    print(markdown(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
