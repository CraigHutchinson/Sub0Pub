#!/usr/bin/env python3
"""Measure Sub0Pub code/RAM footprint per scenario, policy and target. Prints a Markdown report.

Each fp_*.cpp scenario is compiled to an object file (-Os, no exceptions/RTTI) for:
  * host   - the system g++ (x86-64 / arm64)
  * cm33   - arm-none-eabi-g++ -mcpu=cortex-m33 (nRF54 / nRF5340 application core), if installed
and for each dispatch policy. Reported:
  * text/data/bss of the scenario object (what the application pays for that usage)
  * marginal cost of a second subscriber, second publish call site and second Data type
  * per-symbol sizes of the Sub0Pub functions/state instantiated
  * external symbols the object needs at link time (runtime dependencies: TLS, operator delete, mutex...)
  * sizeof(Subscribe<T>) / sizeof(Publish<T>)

Usage: python3 tests/footprint/measure_footprint.py > footprint.md
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
INCLUDE = os.path.normpath(os.path.join(HERE, "..", "..", "include"))

SCENARIOS = OrderedDict([
    ("fp_1type", "1 type: 1 publisher, 1 subscriber, 1 publish site"),
    ("fp_1type_2subs", "+1 subscriber of the same type"),
    ("fp_2sites", "+1 publish call site of the same type"),
    ("fp_2types", "+1 Data type (publisher, subscriber, site)"),
    ("fp_ipc", "1 type forwarded to StreamSerializer"),
])

POLICIES = OrderedDict([
    ("Direct (default)", ["-DNDEBUG"]),
    ("Direct + check", ["-DNDEBUG", "-DSUB0PUB_REENTRANT_CHECK=true"]),
    ("Full (snapshot, cancel, filter)", ["-DNDEBUG", "-DSUB0PUB_REENTRANT_SAFE=true", "-DSUB0PUB_CANCEL=true",
                                         "-DSUB0PUB_FILTER=true"]),
    ("ThreadSafe", ["-DNDEBUG", "-DSUB0PUB_THREAD_SAFE=true"]),
])

AXIS_DIR = os.path.join(HERE, "axis")
# Runtime-broker configuration options, each changed alone from the Full base or the Lean library default
AXIS_SCENARIOS = OrderedDict([
    ("fp_axis_lean", "Lean: Direct, NoContext, NoFilter (the library default)"),
    ("fp_axis_full", "Full: Snapshot, ThreadLocal context, filter"),
    ("fp_axis_direct", "Full, Dispatch=Direct"),
    ("fp_axis_direct_static", "Full, Dispatch=Direct, Context=Static (no TLS)"),
    ("fp_axis_checked", "Full, Dispatch=DirectChecked"),
    ("fp_axis_snapstatic", "Full, Context=Static"),
    ("fp_axis_nocontext", "Full, Context=None (must be rejected: Snapshot needs a context)"),
    ("fp_axis_nofilter", "Full, Filter=off"),
    ("fp_axis_lock", "Full, Lock=spin (RTOS-style yield hook)"),
    ("fp_axis_lockstatic", "Full, Lock=spin + StaticContext (must be rejected)"),
    ("fp_axis_scoped", "Full, Storage=Scoped"),
    ("fp_axis_cap64", "Full, Capacity=64"),
    ("fp_axis_impl", "Full, Implementation=SingleSubscriberBroker"),
    ("fp_axis_route", "Full + 1 Route"),
    ("fp_axis_2types_full", "Full, 2 Data types (marginal cost of a type)"),
    ("fp_axis_2types_lean", "Lean, 2 Data types (marginal cost of a type)"),
])

COMMON = ["-std=c++23", "-Os", "-fno-exceptions", "-fno-rtti", "-ffunction-sections", "-fdata-sections", "-I" + INCLUDE]

TARGETS = OrderedDict()
if shutil.which("g++"):
    TARGETS["host"] = {"cxx": "g++", "size": "size", "nm": "nm", "flags": []}
if shutil.which("arm-none-eabi-g++"):
    TARGETS["cm33"] = {"cxx": "arm-none-eabi-g++", "size": "arm-none-eabi-size", "nm": "arm-none-eabi-nm",
                       "flags": ["-mcpu=cortex-m33", "-mthumb", "-mfloat-abi=hard", "-mfpu=fpv5-sp-d16"]}


def compile_obj(target, scenario, policy_flags, out_dir, src_dir=HERE, extra=()):
    obj = os.path.join(out_dir, scenario + ".o")
    cmd = [target["cxx"], *COMMON, *target["flags"], *policy_flags, *extra, "-c", os.path.join(src_dir, scenario + ".cpp"), "-o", obj]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        first_error = next((l for l in r.stderr.splitlines() if "error" in l), r.stderr.strip()[:120])
        return None, first_error.replace(os.path.dirname(INCLUDE) + os.sep, "").strip()  # repository-relative
    return obj, None


def section_sizes(target, obj):
    out = subprocess.run([target["size"], obj], capture_output=True, text=True, check=True).stdout.splitlines()
    text, data, bss = (int(x) for x in out[1].split()[:3])
    return text, data, bss


def symbols(target, obj):
    out = subprocess.run([target["nm"], "-C", "-S", "--size-sort", obj], capture_output=True, text=True, check=True).stdout
    syms = []
    for line in out.splitlines():
        parts = line.split(None, 3)
        if len(parts) == 4:
            syms.append((int(parts[1], 16), parts[2], parts[3]))
    return syms


def undefined(target, obj):
    out = subprocess.run([target["nm"], "-C", "-u", obj], capture_output=True, text=True, check=True).stdout
    return sorted({l.split(None, 1)[-1].strip() for l in out.splitlines() if l.strip()})


def short(name):
    name = re.sub(r"\(.*\)( const)?$", "()", name)
    return name.replace("sub0::detail::", "").replace("sub0::", "")


def main():
    if not TARGETS:
        sys.exit("no compiler found")
    print("# Sub0Pub footprint report\n")
    for name, t in TARGETS.items():
        ver = subprocess.run([t["cxx"], "--version"], capture_output=True, text=True).stdout.splitlines()[0]
        print(f"- **{name}**: `{ver}` `{' '.join(COMMON[:4] + t['flags'])}`")
    print()

    results = {}
    with tempfile.TemporaryDirectory() as tmp:
        for tname, target in TARGETS.items():
            for pname, pflags in POLICIES.items():
                for scen in SCENARIOS:
                    d = os.path.join(tmp, tname, re.sub(r"\W+", "_", pname))
                    os.makedirs(d, exist_ok=True)
                    obj, err = compile_obj(target, scen, pflags, d)
                    if obj is None:
                        results[(tname, pname, scen)] = {"error": err}
                        continue
                    results[(tname, pname, scen)] = {
                        "size": section_sizes(target, obj),
                        "syms": symbols(target, obj),
                        "undef": undefined(target, obj),
                    }

    for tname in TARGETS:
        print(f"## Target: {tname}\n")
        print("### Object size (text / data / bss bytes)\n")
        print("| Scenario | " + " | ".join(POLICIES) + " |")
        print("|---|" + "---:|" * len(POLICIES))
        for scen, desc in SCENARIOS.items():
            cells = []
            for pname in POLICIES:
                r = results[(tname, pname, scen)]
                cells.append("n/a" if "error" in r else "{} / {} / {}".format(*r["size"]))
            print(f"| {desc} | " + " | ".join(cells) + " |")
        print()

        print("### Marginal text bytes\n")
        print("| Added usage | " + " | ".join(POLICIES) + " |")
        print("|---|" + "---:|" * len(POLICIES))
        for scen in ("fp_1type_2subs", "fp_2sites", "fp_2types"):
            cells = []
            for pname in POLICIES:
                a, b = results[(tname, pname, "fp_1type")], results[(tname, pname, scen)]
                cells.append("n/a" if "error" in a or "error" in b else f"{b['size'][0] - a['size'][0]:+d}")
            print(f"| {SCENARIOS[scen]} | " + " | ".join(cells) + " |")
        print()

        print("### Sub0Pub symbols, 1 type (bytes)\n")
        names = []
        for pname in POLICIES:
            r = results[(tname, pname, "fp_1type")]
            for size, kind, name in r.get("syms", []):
                if "sub0::" in name and short(name) not in names:
                    names.append(short(name))
        print("| Symbol | " + " | ".join(POLICIES) + " |")
        print("|---|" + "---:|" * len(POLICIES))
        for n in names:
            cells = []
            for pname in POLICIES:
                r = results[(tname, pname, "fp_1type")]
                total = sum(s for s, k, nm in r.get("syms", []) if short(nm) == n)
                cells.append(str(total) if total else "-")
            print(f"| `{n}` | " + " | ".join(cells) + " |")
        print()

        print("### sizeof (bytes)\n")
        r = results[(tname, "Direct (default)", "fp_1type")]
        for size, kind, name in r.get("syms", []):
            if name.startswith("fp_sizeof_"):
                print(f"- `{name[len('fp_sizeof_'):]}<T>`: {size}")
        print()

        print("### Link-time dependencies, 1 type (undefined symbols)\n")
        for pname in POLICIES:
            r = results[(tname, pname, "fp_1type")]
            if "error" in r:
                print(f"- **{pname}**: does not compile: `{r['error']}`")
            else:
                print(f"- **{pname}**: " + ", ".join(f"`{u}`" for u in r["undef"]))
        print()


    # Per-type configuration (docs/DESIGN.md): the policy is bound per Data type, not per build
    if os.path.isdir(AXIS_DIR):
        print("## Runtime broker configuration, one option at a time\n")
        print("Same usage as `fp_1type` (1 type, 1 publisher, 1 subscriber, 1 publish site); compare with "
              "the Direct (default) column above.\n")
        with tempfile.TemporaryDirectory() as tmp:
            for tname, target in TARGETS.items():
                print(f"### Target: {tname}\n")
                print("| Configuration | text / data / bss | `Broker::publish()` | sizeof Subscribe / Publish | Link-time dependencies |")
                print("|---|---:|---:|---:|---|")
                for scen, desc in AXIS_SCENARIOS.items():
                    obj, err = compile_obj(target, scen, ["-DNDEBUG"], tmp, AXIS_DIR, ["-I" + AXIS_DIR])
                    if obj is None:
                        print(f"| {desc} | n/a: `{err}` | | | |")
                        continue
                    syms = symbols(target, obj)
                    pub = sum(sz for sz, k, n in syms if "Broker" in n and "::publish(" in n)
                    sizes = {n[len("fp_sizeof_"):]: sz for sz, k, n in syms if n.startswith("fp_sizeof_")}
                    undef = ", ".join(f"`{u}`" for u in undefined(target, obj))
                    more = "".join(f", {k} {v}" for k, v in sizes.items() if k not in ("Subscribe", "Publish"))
                    print(f"| {desc} | {' / '.join(map(str, section_sizes(target, obj)))} | {pub or 'inlined'} | "
                          f"{sizes.get('Subscribe', '?')} / {sizes.get('Publish', '?')}{more} | {undef} |")
                print()


if __name__ == "__main__":
    main()
