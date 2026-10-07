#!/usr/bin/env python3
"""Collapse evidence tool: final-link evidence that a Sub0Pub coding pattern compiles away (docs/EVIDENCE.md).

For every case in tests/collapse/cases/<case>/ and every variant file in it, in both forms
(observable-work, removable-work), on every available named build, this links a real executable
(driver.cpp + variant) and records:

  runtime (host builds, run natively and under callgrind):
    checksum                    must equal handwritten's (equal observable behaviour)
    setup / publish / teardown  instructions (publish is per publication)
  final image (all builds; the MSVC build reads the PE image with dumpbin and the linker map, see below):
    publish path                instructions of collapse_publish plus every function reachable from it by
                                direct calls, direct/indirect call counts, and the callees retained
    text / data / bss           of the whole image; deltas against handwritten are reported
    retained Sub0Pub            code/state symbols from namespace sub0 left in the image
    dependencies                TLS, operator delete, __cxa_pure_virtual, static initialisers
  and a verdict per criterion against handwritten (the equal-work reference) of the same build and form.

MSVC (`msvc-O2`, `msvc-O2-lto`): the same evidence from the final PE image. cl /O2 /GS- links with /MAP and
/DEBUG /OPT:REF /OPT:NOICF /INCREMENTAL:NO; `dumpbin /disasm` gives the publish path, the map gives section sizes
(text = code + read-only data, like GNU `size`; init = `.CRT$XC*` initialiser pointers) and symbol sizes (distance
to the next public in the section). Functions taken from a library (CRT) rather than a user object are external,
like PLT stubs. Windows has no callgrind, so instruction counts (`instr`) are not measured there: the static
publish path and the checksum are.

Usage:
  python3 tests/collapse/collapse_evidence.py [--case NAME] [--build NAME ...] [--json OUT.json] > report.md
  python3 tests/collapse/collapse_evidence.py --self-test     # check the publish-path parser
  python3 tests/collapse/collapse_evidence.py --budgets tests/collapse/budgets.json        # regression gate (CI)
  python3 tests/collapse/collapse_evidence.py --write-budgets tests/collapse/budgets.json  # record new budgets
Exit status is non-zero if any variant's checksum differs from its reference (behaviour broken), if any build,
run or measurement fails, if a declared reference is missing, or if the case/build selection is empty. Cost
criterion verdicts are reported, not enforced: the runtime-registry variants are expected to fail them; that is the
price of runtime subscription.

Regression gate (--budgets): every public-API variant (`sub0_*`, `sub0pub_virtual*`) has a budget per build and form
for each metric's delta against its reference (publish/setup/teardown instructions, publish path, indirect calls,
text, RAM, static initialisation, added dependencies). Exceeding a budget, or a public-API variant with no budget,
fails the tool. Budgets are the measured deltas when recorded, never tighter than the criteria's tolerances; a
deliberate change re-records them with --write-budgets and commits the file with the change. The references are
not gated.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import OrderedDict
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
INCLUDE = os.path.join(ROOT, "include")
CASES_DIR = os.path.join(HERE, "cases")
PUBLISHES = 1000  # driver.cpp kPublishes
REFERENCE = "handwritten"

COMMON = ["-std=c++23", "-DNDEBUG", "-ffunction-sections", "-fdata-sections", "-I" + HERE, "-I" + INCLUDE]

# Named builds. host builds run (checksum + callgrind); cross builds are analysed from the final ELF only.
BUILDS = OrderedDict([
    ("gcc-O2", {"cxx": "g++", "flags": ["-O2"], "ldflags": ["-Wl,--gc-sections", "-Wl,-z,now"],
                "objdump": "objdump", "nm": "nm", "size": "size", "run": True, "arch": "x86"}),
    ("clang-O2", {"cxx": "clang++", "flags": ["-O2"], "ldflags": ["-Wl,--gc-sections", "-Wl,-z,now"],
                  "objdump": "objdump", "nm": "nm", "size": "size", "run": True, "arch": "x86"}),
    ("cm33-gcc-Os", {"cxx": "arm-none-eabi-g++",
                     "flags": ["-Os", "-mcpu=cortex-m33", "-mthumb", "-mfloat-abi=hard", "-mfpu=fpv5-sp-d16",
                               "-fno-exceptions", "-fno-rtti", "-DCOLLAPSE_NO_STDIO"],
                     "ldflags": ["--specs=nano.specs", "--specs=nosys.specs", "-Wl,--gc-sections"],
                     "support": ["support/bare_metal_tls.cpp"],
                     "objdump": "arm-none-eabi-objdump", "nm": "arm-none-eabi-nm", "size": "arm-none-eabi-size",
                     "run": False, "arch": "arm"}),
])

# LTO counterparts: only meaningful (and only run) for cases whose variants span several translation units
for _name in list(BUILDS):
    _cfg = dict(BUILDS[_name])
    _cfg["flags"] = _cfg["flags"] + ["-flto"]
    _cfg["ldflags"] = _cfg["ldflags"] + ["-flto"] + [f for f in _cfg["flags"] if f.startswith("-O")]
    _cfg["multi_tu_only"] = True
    BUILDS[_name + "-lto"] = _cfg


# MSVC named builds are added when a Windows host has (or can locate) the toolchain. /GS- leaves out the stack
# cookie (the ELF builds use no stack protector); /Gy /Gw are the equivalents of -ffunction-sections
# -fdata-sections; /OPT:NOICF keeps identical functions apart, as gc-sections does.
MSVC_COMMON = ["/nologo", "/W3", "/std:c++latest", "/DNDEBUG", "/Gy", "/Gw", "/GS-", "/EHsc", "/Zc:preprocessor", "/Zi"]
BUILDS["msvc-O2"] = {"kind": "msvc", "cxx": "cl", "flags": ["/O2"],
                     "ldflags": ["/INCREMENTAL:NO", "/OPT:REF", "/OPT:NOICF", "/DEBUG"],
                     "objdump": "dumpbin", "run": True, "arch": "x86"}
BUILDS["msvc-O2-lto"] = dict(BUILDS["msvc-O2"], flags=["/O2", "/GL"], ldflags=BUILDS["msvc-O2"]["ldflags"] + ["/LTCG"],
                             multi_tu_only=True)

FORMS = OrderedDict([("observable", 1), ("removable", 0)])

# Tolerances for "equivalent to handwritten" verdicts
INSTR_TOLERANCE = 2          # extra instructions per publication
PATH_TOLERANCE = 2           # extra static instructions on the publish path

DEPENDENCY_MARKERS = OrderedDict([
    ("TLS", re.compile(r"^(__aeabi_read_tp|__tls_get_addr)$")),
    ("operator delete", re.compile(r"^_ZdlPv")),
    ("pure virtual", re.compile(r"^__cxa_pure_virtual$")),
    ("atexit", re.compile(r"^(__cxa_atexit|__aeabi_atexit|atexit)$")),
])


# The same markers as decorated x64 MSVC names (the linker map lists decorated public names)
DEPENDENCY_MARKERS_MSVC = OrderedDict([
    ("TLS", re.compile(r"^(_tls_index|__tls_array)$")),
    ("operator delete", re.compile(r"^\?\?3@YAXPEAX(_K)?@Z$")),
    ("pure virtual", re.compile(r"^_purecall$")),
    ("atexit", re.compile(r"^(atexit|_crt_atexit|_onexit|_register_onexit_function)$")),
])

SUB0_NAMESPACE = re.compile(r"(^|[^\w])sub0::")
SUB0_NAMESPACE_MSVC = re.compile(r"@sub0@@")   # decorated: `?f@Widget@sub0@@...`


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def is_msvc(build_cfg):
    return build_cfg.get("kind") == "msvc"


def ensure_msvc_environment():
    """On Windows, when cl is not on PATH, import the newest Visual Studio x64 build environment (vcvars64.bat).
    A no-op elsewhere and when cl is already available (developer prompt)."""
    if sys.platform != "win32" or shutil.which("cl"):
        return
    # vswhere knows which installation is newest; directory names do not sort (`2022` is older than `18`).
    # -prerelease: a preview or Insiders channel install (VS 18 here) is otherwise invisible to it
    installer = os.path.join(os.environ.get("ProgramFiles(x86)", ""), "Microsoft Visual Studio", "Installer", "vswhere.exe")
    installs = []
    if os.path.isfile(installer):
        installs = run([installer, "-latest", "-prerelease", "-products", "*", "-requires",
                        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).stdout.splitlines()
    candidates = [os.path.join(i.strip(), "VC", "Auxiliary", "Build", "vcvars64.bat") for i in installs if i.strip()]
    for vcvars in (c for c in candidates if os.path.isfile(c)):
        r = run(f'"{vcvars}" >nul 2>nul && set', shell=True)
        # cmd preserves spellings such as Path/Include; Windows lookup is case-insensitive.
        env = {key.upper(): value for line in r.stdout.splitlines() if "=" in line
               for key, value in [line.split("=", 1)] if key}
        if r.returncode == 0 and env.get("PATH"):
            os.environ.update(env)
            if shutil.which("cl"):
                return


def available_builds():
    if any(is_msvc(b) for b in BUILDS.values()):
        ensure_msvc_environment()
    return OrderedDict((n, b) for n, b in BUILDS.items() if shutil.which(b["cxx"]) and shutil.which(b["objdump"]))


def compiler_version(build_cfg):
    if is_msvc(build_cfg):  # the banner goes to stderr, and the exit status of a bare `cl` is non-zero
        r = run([build_cfg["cxx"]])
        return next(l.strip() for l in (r.stderr + r.stdout).splitlines() if l.strip())
    return run([build_cfg["cxx"], "--version"]).stdout.splitlines()[0]


def discover_cases(only=None):
    cases = OrderedDict()
    for name in sorted(os.listdir(CASES_DIR)):
        d = os.path.join(CASES_DIR, name)
        if not os.path.isdir(d) or (only and name != only):
            continue
        # A variant is a single file (<variant>.cpp) or a directory of translation units (<variant>/*.cpp)
        variants = sorted(f[:-4] for f in os.listdir(d) if f.endswith(".cpp"))
        variants += sorted(f for f in os.listdir(d) if os.path.isdir(os.path.join(d, f)))
        if REFERENCE not in variants:
            sys.exit(f"case {name}: missing {REFERENCE}")
        # Extra equal-work references (handwritten_<kind>, e.g. handwritten_runtime) are listed first; each is
        # itself compared with `handwritten`, and a variant selects one with `// COLLAPSE_REFERENCE: <name>`
        refs = [REFERENCE] + [v for v in variants if v.startswith(REFERENCE + "_")]
        cases[name] = refs + [v for v in variants if v not in refs]
    return cases


def select_cases(profile, only=None):
    cases = discover_cases(only)
    if profile == "full":
        return cases
    smoke_file = Path(__file__).with_name("smoke_cases.txt")
    # One case name per line; blank lines and # comments are skipped, anything else is an error (as in CMake)
    smoke_names = [line.strip() for line in smoke_file.read_text().splitlines()
                   if line.strip() and not line.strip().startswith("#")]
    malformed = [name for name in smoke_names if not re.fullmatch(r"[a-z_]+", name)]
    if malformed:
        raise ValueError(f"malformed smoke case lines: {', '.join(malformed)}")
    missing_cases = sorted(set(smoke_names) - set(discover_cases()))
    if missing_cases:
        raise ValueError(f"smoke cases missing: {', '.join(missing_cases)}")
    return OrderedDict((name, variants) for name, variants in cases.items() if name in smoke_names)


def variant_sources(case, variant):
    path = os.path.join(CASES_DIR, case, variant)
    if os.path.isdir(path):
        return sorted(os.path.join(path, f) for f in os.listdir(path) if f.endswith(".cpp"))
    return [path + ".cpp"]


REF_MARKER = re.compile(r"^//\s*COLLAPSE_REFERENCE:\s*(\w+)\s*$")


def variant_reference(case, variant):
    """The equal-work reference a variant is judged against: `handwritten` unless one of its sources' leading
    comment lines names another reference of the same case (`// COLLAPSE_REFERENCE: handwritten_runtime`)."""
    if variant == REFERENCE:
        return None
    for src in variant_sources(case, variant):
        try:
            with open(src) as fh:
                head = [fh.readline() for _ in range(4)]
        except OSError:
            continue
        for line in head:
            m = REF_MARKER.match(line.strip())
            if m:
                return m.group(1)
    return REFERENCE


def multi_tu(case, variants):
    return any(os.path.isdir(os.path.join(CASES_DIR, case, v)) for v in variants)


def build_msvc(build_cfg, case, variant, observable, out_dir):
    stem = os.path.join(out_dir, f"{case}-{variant}-{observable}")
    exe = stem + ".exe"
    sources = variant_sources(case, variant)
    obj_dir = stem + "_obj"
    os.makedirs(obj_dir, exist_ok=True)
    cmd = [build_cfg["cxx"], *MSVC_COMMON, *build_cfg["flags"], f"/DCOLLAPSE_OBSERVABLE={observable}",
           "/I" + HERE, "/I" + INCLUDE, f"/Fo{obj_dir}\\", f"/Fd{stem}_cl.pdb",
           os.path.join(HERE, "driver.cpp"), *sources, *(os.path.join(HERE, f) for f in build_cfg.get("support", [])),
           f"/Fe{exe}", "/link", *build_cfg["ldflags"], f"/MAP:{stem}.map", f"/PDB:{stem}.pdb"]
    r = run(cmd)
    if r.returncode != 0:
        lines = (r.stdout + r.stderr).splitlines()
        err = next((l for l in lines if re.search(r"\b(error|fatal error) [A-Z]+\d+", l)), (r.stdout + r.stderr).strip()[:200])
        return None, err
    return exe, None


def build(build_cfg, case, variant, observable, out_dir):
    if is_msvc(build_cfg):
        return build_msvc(build_cfg, case, variant, observable, out_dir)
    exe = os.path.join(out_dir, f"{case}-{variant}-{observable}.elf")
    mapfile = exe[:-4] + ".map"
    sources = variant_sources(case, variant)
    cmd = [build_cfg["cxx"], *COMMON, *build_cfg["flags"], f"-DCOLLAPSE_OBSERVABLE={observable}",
           os.path.join(HERE, "driver.cpp"), *sources,
           *(os.path.join(HERE, f) for f in build_cfg.get("support", [])),
           "-o", exe, *build_cfg["ldflags"], f"-Wl,-Map={mapfile}"]
    r = run(cmd)
    if r.returncode != 0:
        lines = r.stderr.splitlines()
        err = next((l.split(": ", 1)[-1] for l in lines if "undefined reference" in l),
                   next((l for l in lines if "error" in l), r.stderr.strip()[:200]))
        return None, err
    return exe, None


def checksum(exe):
    """(checksum, error): the run must exit 0 and print its checksum."""
    r = run([exe])
    m = re.search(r"checksum (.*)", r.stdout)
    if r.returncode != 0 or not m:
        return None, f"run failed: exit={r.returncode} {r.stderr.strip()[:200]}"
    return m.group(1).strip(), None


def callgrind(exe):
    phases = {}
    with tempfile.TemporaryDirectory() as tmp:
        result = run(["valgrind", "--tool=callgrind", "--callgrind-out-file=" + os.path.join(tmp, "cg.%p"), exe])
        if result.returncode != 0:
            return phases, f"callgrind failed: exit={result.returncode} {result.stderr.strip()[:200]}"
        for f in os.listdir(tmp):
            label, total = None, None
            with open(os.path.join(tmp, f)) as fh:
                for line in fh:
                    if line.startswith("desc: Trigger: Client Request: "):
                        label = line.split("Client Request: ", 1)[1].strip()
                    elif line.startswith("summary:"):
                        total = int(line.split()[1])
            if label and total is not None:
                phases[label] = total
    if "publish" in phases:
        phases["publish"] = phases["publish"] / PUBLISHES
    missing = [p for p in ("setup", "publish", "teardown") if p not in phases]
    return phases, (f"callgrind: missing phases {missing}" if missing else None)


def parse_objdump(text):
    """Parse `objdump -d --no-show-raw-insn -C` output.

    Returns (funcs, names): funcs maps each function's start address to [(mnemonic, operands)], names maps the
    start address to its demangled name. Functions are keyed by address, never by name: demangled names contain
    '<', '>', '+' and spaces, and two distinct functions can share a demangled name (anonymous namespaces in
    different TUs, constructor aliases)."""
    funcs, names, current = {}, {}, None
    for line in text.splitlines():
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
        if m:
            current = int(m.group(1), 16)
            funcs[current] = []
            names[current] = m.group(2)
            continue
        m = re.match(r"^\s+[0-9a-f]+:\s+(\S+)\s*(.*)$", line)
        if m and current is not None:
            funcs[current].append((m.group(1), m.group(2)))
    return funcs, names


def disassemble(build_cfg, exe):
    if is_msvc(build_cfg):
        text = run([build_cfg["objdump"], "/nologo", "/disasm", exe], encoding="utf-8", errors="replace").stdout
        return parse_dumpbin(text, parse_map(read_map(exe)))
    return parse_objdump(run([build_cfg["objdump"], "-d", "--no-show-raw-insn", "-C", exe]).stdout)


# ---- MSVC: linker map and dumpbin /disasm ----------------------------------------------------------------------

MAP_SECTION = re.compile(r"^ ([0-9a-f]{4}):([0-9a-f]{8}) ([0-9a-f]{8})H (\S+)\s+(CODE|DATA)\s*$")
MAP_PUBLIC = re.compile(r"^ ([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{16})\s+((?:[fi]\s+)*)(\S+)\s*$")
DUMPBIN_LABEL = re.compile(r"^(\S.*):$")
DUMPBIN_INSN = re.compile(r"^\s+([0-9A-F]{16}): (?:[0-9A-F]{2} ?)+\s+(\S.*)$")
X64_REGISTER = re.compile(r"^(r(?:[0-9]+|[abcd]x|[sd]i|[sb]p)[dwb]?|e?[abcd]x|e?[sd]i|e?[sb]p|[abcd][lh]|[sd]il|[sb]pl|[cdefgs]s)$")
# Padding the disassembler attributes to the function before it (a symbol table gives starts, not ends)
DUMPBIN_PADDING = ("nop", "xchg", "int", "data16")


def read_map(exe):
    with open(os.path.splitext(exe)[0] + ".map", encoding="utf-8", errors="replace") as fh:
        return fh.read()


def parse_map(text):
    """Parse an MSVC linker map: sections [(segment, offset, length, name)] and symbols [(segment, offset, name,
    address, object)] from `Publics by Value` and `Static symbols` (the object is `lib:obj` for library code), and
    the names the map flags `f` (functions)."""
    sections_, symbols_, functions = [], [], set()
    for line in text.splitlines():
        m = MAP_SECTION.match(line)
        if m:
            sections_.append((int(m.group(1), 16), int(m.group(2), 16), int(m.group(3), 16), m.group(4)))
            continue
        m = MAP_PUBLIC.match(line)
        if m and int(m.group(1), 16) != 0:
            symbols_.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3), int(m.group(4), 16), m.group(6)))
            if "f" in m.group(5).split():
                functions.add(m.group(3))
    return {"sections": sections_, "symbols": symbols_, "functions": functions}


def map_symbol_sizes(info):
    """{name: bytes}: a symbol runs to the next symbol of its segment (or the end of the segment). Includes up to
    15 bytes of alignment padding, the price of a map that records starts only."""
    ends = {}
    for seg, off, length, _ in info["sections"]:
        ends[seg] = max(ends.get(seg, 0), off + length)
    by_seg = {}
    for seg, off, name, _, _ in info["symbols"]:
        by_seg.setdefault(seg, []).append((off, name))
    sizes = {}
    for seg, syms in by_seg.items():
        offsets = sorted({o for o, _ in syms})
        following = {o: (offsets[i + 1] if i + 1 < len(offsets) else ends.get(seg, o)) for i, o in enumerate(offsets)}
        for off, name in syms:
            sizes[name] = sizes.get(name, 0) + following[off] - off
    return sizes


def parse_dumpbin(text, info):
    """Parse `dumpbin /disasm` (on an image linked with /DEBUG) into the shape parse_objdump returns.

    Only code from user objects is a function of the path: a label the map attributes to a library object (CRT,
    `lib:obj`) is external, like a PLT stub. Branch operands are rewritten into objdump's form so the shared
    publish-path walk needs no MSVC knowledge: `<hex> <name>` for a call into user code, `0 <name>` for an external
    callee or an import, `*<operand>` for an indirect call or jump, plain hex for a branch inside a function."""
    library = {name for _, _, name, _, obj in info["symbols"] if ":" in obj}
    raw, starts, label, current = OrderedDict(), {}, None, None
    for line in text.splitlines():
        m = DUMPBIN_INSN.match(line)
        if m:
            addr = int(m.group(1), 16)
            if label is not None:
                current, starts[label] = addr, addr
                raw[current] = (label, [])
                label = None
            mm = re.match(r"([a-z][a-z0-9]*)(?:\s+(.*))?$", m.group(2).rstrip())
            if mm and current is not None:
                raw[current][1].append((mm.group(1), (mm.group(2) or "").strip()))
            continue
        m = DUMPBIN_LABEL.match(line)
        if m and not line.startswith(("Dump of", "File Type")):
            label = m.group(1)
    funcs, names = {}, {}
    for addr, (name, insns) in raw.items():
        if name in library:
            continue
        while insns and insns[-1][0] in DUMPBIN_PADDING:
            insns.pop()
        names[addr] = name
        funcs[addr] = [(mn, msvc_branch_operand(mn, ops, starts, library)) for mn, ops in insns]
    return funcs, names


def msvc_branch_operand(mnemonic, operands, starts, library):
    """One instruction's operands in objdump's branch notation (see parse_dumpbin); others pass through."""
    if not (mnemonic.startswith("call") or mnemonic.startswith("jmp")):
        return operands
    if re.fullmatch(r"[0-9A-F]{16}", operands):
        return f"{int(operands, 16):x}"
    imported = re.fullmatch(r"qword ptr \[(__imp_\S+)\]", operands)
    if imported:
        return f"0 <{imported.group(1)}>"
    if re.fullmatch(r"[A-Za-z_?$][\w?$@.]*", operands) and not X64_REGISTER.match(operands):
        if operands in starts and operands not in library:
            return f"{starts[operands]:x} <{operands}>"
        return f"0 <{operands}>"
    return "*" + operands


def msvc_section_sizes(info):
    """By section name, as GNU size counts an ELF: data = `.data*` and `.tls*` (writable), bss = `.bss`, text =
    everything else (code, read-only data, and the unwind tables `.pdata`/`.xdata`, as `.eh_frame` is text in an
    ELF). The debug directory (`.rdata$zzzdbg`, which holds the PDB path) is left out: its size follows the output
    file name, not the code. init = the pointers in `.CRT$XC*` (dynamic initialisers), without the begin/end
    markers."""
    out = {"text": 0, "data": 0, "bss": 0, "init_array": 0}
    for _, _, length, name in info["sections"]:
        if name == ".rdata$zzzdbg":
            continue
        if name.startswith((".data", ".tls")):
            out["data"] += length
        elif name == ".bss":
            out["bss"] += length
        else:
            out["text"] += length
        if re.match(r"\.CRT\$XC(?![AZ])", name):
            out["init_array"] += length
    return out


_undname_cache = {}


def undecorate(names):
    """MSVC-decorated names -> readable, one undname call per batch (cached); other names pass through."""
    todo = [n for n in names if n.startswith("?") and n not in _undname_cache]
    for i in range(0, len(todo), 40):
        chunk = todo[i:i + 40]
        out = run(["undname", *chunk]).stdout
        decoded = re.findall(r'^is :- "(.*)"$', out, re.M)
        for name, text in zip(chunk, decoded if len(decoded) == len(chunk) else chunk):
            _undname_cache[name] = text
    return [_undname_cache.get(n, n) for n in names]


def is_call(arch, mnemonic):
    return (mnemonic.startswith("call") if arch == "x86" else mnemonic in ("bl", "blx"))


def branch_address(operands):
    """The numeric target of a direct branch (`call 401126 <ns::f<A<B>>(A<B> const&)>`), or None if indirect.

    The target is resolved from objdump's numeric address, not from the symbol text. The earlier parser cut the
    demangled name at the first '>' or '+', so a call to `sub0::detail::deliver<R, M>(R&, M const&)` became an
    unresolvable name and its body was left off the publish path."""
    m = re.match(r"^(?:0x)?([0-9a-f]+)(?:\s|$)", operands)
    return int(m.group(1), 16) if m else None


def is_plt(name):
    return "@plt" in name


def strip_plt(label):
    """`memcpy@plt` -> `memcpy`. Only the PLT suffix goes: decorated MSVC names contain '@' throughout."""
    return re.sub(r"@plt(\+0x[0-9a-f]+)?$", "", label)


def is_tail_jump(arch, mnemonic, operands, funcs, current):
    """A branch to the start of another function (a tail call). Branches inside a function are not calls."""
    if not ((arch == "x86" and mnemonic.startswith("jmp")) or (arch == "arm" and mnemonic in ("b", "b.w", "b.n"))):
        return None
    target = branch_address(operands)
    if target is None or target == current or target not in funcs:
        return None
    return target


def publish_path(build_cfg, funcs, names=None, root="collapse_publish"):
    """Instructions and calls on the publication path: collapse_publish plus directly reachable functions."""
    arch = build_cfg["arch"]
    if names is None:  # legacy shape {name: insns}
        names = {i: n for i, n in enumerate(funcs)}
        funcs = {i: funcs[n] for i, n in names.items()}
    start = next((a for a, n in names.items() if n == root), None)
    seen, stack = [], [start] if start is not None else []
    instructions = direct = indirect = 0
    external = set() if start is not None else {root}
    while stack:
        f = stack.pop()
        if f in seen:
            continue
        seen.append(f)
        for mnemonic, operands in funcs[f]:
            instructions += 1
            if is_call(arch, mnemonic):
                target = branch_address(operands)
                if target is None or "*" in operands or (arch == "arm" and mnemonic == "blx" and re.match(r"r\d", operands)):
                    indirect += 1
                else:
                    direct += 1
                    if target in funcs and not is_plt(names[target]):
                        stack.append(target)
                    else:
                        label = names.get(target) or (re.search(r"<(.*)>\s*$", operands) or [None, hex(target)])[1]
                        external.add(strip_plt(label))
            else:
                tail = is_tail_jump(arch, mnemonic, operands, funcs, f)
                if tail is not None:
                    direct += 1
                    if is_plt(names[tail]):
                        external.add(strip_plt(names[tail]))
                    else:
                        stack.append(tail)
                elif arch == "x86" and mnemonic.startswith("jmp") and "*" in operands:
                    indirect += 1
                elif arch == "x86" and mnemonic.startswith("jmp") and branch_address(operands) == 0 and "<" in operands:
                    # an external tail call: MSVC notation for a library function or an import (parse_dumpbin)
                    direct += 1
                    external.add(re.search(r"<(.*)>\s*$", operands).group(1))
    return {"instructions": instructions, "direct_calls": direct, "indirect_calls": indirect,
            "functions": [names[f] for f in seen], "external": sorted(external)}


SELF_TEST_X86 = """
0000000000401000 <collapse_publish>:
  401000:	push   %rbx
  401001:	call   401100 <void sub0::detail::deliver<(anonymous namespace)::Controller, (anonymous namespace)::Sample>((anonymous namespace)::Controller&, (anonymous namespace)::Sample const&)>
  401006:	call   401200 <memcpy@plt>
  40100b:	call   *%rax
  40100d:	jne    401000 <collapse_publish>
  40100f:	pop    %rbx
  401010:	jmp    401300 <sub0::Wiring<A<B<C> >, D>::publish<E>(E const&) const+0x0>

0000000000401100 <void sub0::detail::deliver<(anonymous namespace)::Controller, (anonymous namespace)::Sample>((anonymous namespace)::Controller&, (anonymous namespace)::Sample const&)>:
  401100:	mov    (%rdi),%eax
  401102:	ret

0000000000401200 <memcpy@plt>:
  401200:	jmp    *0x2000(%rip)

0000000000401300 <sub0::Wiring<A<B<C> >, D>::publish<E>(E const&) const>:
  401300:	add    $0x1,%eax
  401303:	jmp    401305 <sub0::Wiring<A<B<C> >, D>::publish<E>(E const&) const+0x5>
  401305:	ret
"""

SELF_TEST_ARM = """
00008000 <collapse_publish>:
    8000:	push	{r4, lr}
    8002:	bl	8100 <sub0::StaticWiring<&(anonymous namespace)::a, &(anonymous namespace)::b>::publish<S>(S const&)>
    8006:	blx	r3
    8008:	b.w	8200 <operator>>(A const&, B const&)>

00008100 <sub0::StaticWiring<&(anonymous namespace)::a, &(anonymous namespace)::b>::publish<S>(S const&)>:
    8100:	bx	lr

00008200 <operator>>(A const&, B const&)>:
    8200:	movs	r0, #1
    8202:	bx	lr
"""


SELF_TEST_MSVC_MAP = """
 Start         Length     Name                   Class
 0001:00000000 00000300H .text$mn                CODE
 0002:00000000 00000100H .rdata                  DATA
 0002:00000100 00000008H .CRT$XCA                DATA
 0002:00000108 00000008H .CRT$XCU                DATA
 0002:00000110 00000008H .CRT$XCZ                DATA
 0003:00000000 00000040H .data                   DATA
 0003:00000040 00000020H .bss                    DATA
 0002:00000118 00000040H .rdata$zzzdbg           DATA
 0004:00000000 00000018H .pdata                  DATA

  Address         Publics by Value              Rva+Base               Lib:Object

 0001:00000000       collapse_publish           0000000140001000 f   variant.obj
 0001:00000040       ?deliver@detail@sub0@@YAXAEAUController@?A0x1@@AEBUSample@2@@Z 0000000140001040 f   variant.obj
 0001:00000060       memcpy                     0000000140001060 f   MSVCRT:memcpy.obj
 0001:00000080       ?publish@?$Wiring@UA@@@sub0@@QEBAXAEBUE@@@Z 0000000140001080 f   variant.obj
 0003:00000000       ?g_state@collapse@@3IA     0000000140003000     driver.obj
 0003:00000004       ?g_args@collapse@@3IA      0000000140003004     driver.obj
"""

SELF_TEST_MSVC = """
Dump of file t.exe

File Type: EXECUTABLE IMAGE

collapse_publish:
  0000000140001000: 53                 push        rbx
  0000000140001001: E8 3A 00 00 00     call        ?deliver@detail@sub0@@YAXAEAUController@?A0x1@@AEBUSample@2@@Z
  0000000140001006: E8 55 00 00 00     call        memcpy
  000000014000100B: FF 15 00 20 00 00  call        qword ptr [__imp_free]
  0000000140001011: FF D0              call        rax
  0000000140001013: FF 50 10           call        qword ptr [rax+10h]
  0000000140001016: 74 E8              je          0000000140001000
  0000000140001018: 5B                 pop         rbx
  0000000140001019: E9 62 00 00 00     jmp         ?publish@?$Wiring@UA@@@sub0@@QEBAXAEBUE@@@Z
  000000014000101E: E9 3D 00 00 00     jmp         memcpy
  0000000140001023: FF 25 00 20 00 00  jmp         qword ptr [__imp_free]
  0000000140001029: CC CC CC CC CC CC                                ......
?deliver@detail@sub0@@YAXAEAUController@?A0x1@@AEBUSample@2@@Z:
  0000000140001040: 8B 07              mov         eax,dword ptr [rcx]
  0000000140001042: C3                 ret
  0000000140001043: 0F 1F 44 00 00     nop         dword ptr [rax+rax]
memcpy:
  0000000140001060: F3 A4              rep movs    byte ptr [rdi],byte ptr [rsi]
  0000000140001062: C3                 ret
?publish@?$Wiring@UA@@@sub0@@QEBAXAEBUE@@@Z:
  0000000140001080: 83 C0 01           add         eax,1
  0000000140001083: EB 00              jmp         0000000140001085
  0000000140001085: C3                 ret
"""


def self_test():
    """Checks the publish-path walk on synthetic objdump output: nested-template callees, a PLT stub, an
    indirect call, a tail call whose target name has an offset suffix, an intra-function branch, and (Arm) a
    tail call to an operator whose name contains '>'."""
    x86 = publish_path({"arch": "x86"}, *parse_objdump(SELF_TEST_X86))
    assert x86["instructions"] == 7 + 2 + 3, x86
    assert x86["direct_calls"] == 3 and x86["indirect_calls"] == 1, x86
    assert x86["external"] == ["memcpy"], x86
    assert any(f.startswith("void sub0::detail::deliver<") for f in x86["functions"]), x86
    arm = publish_path({"arch": "arm"}, *parse_objdump(SELF_TEST_ARM))
    assert arm["instructions"] == 4 + 1 + 2 and arm["direct_calls"] == 2 and arm["indirect_calls"] == 1, arm
    assert "operator>>(A const&, B const&)" in arm["functions"], arm
    info = parse_map(SELF_TEST_MSVC_MAP)
    msvc = publish_path({"arch": "x86"}, *parse_dumpbin(SELF_TEST_MSVC, info))
    # collapse_publish 11 + deliver 2 (padding nop dropped) + Wiring::publish 3; memcpy (library) is external, and
    # so are the tail jumps to it and to an import
    assert msvc["instructions"] == 11 + 2 + 3, msvc
    assert msvc["direct_calls"] == 6 and msvc["indirect_calls"] == 2, msvc
    assert msvc["external"] == ["__imp_free", "memcpy"], msvc
    assert msvc_section_sizes(info) == {"text": 0x430, "data": 0x40, "bss": 0x20, "init_array": 8}, msvc_section_sizes(info)
    sizes = map_symbol_sizes(info)
    assert sizes["?g_state@collapse@@3IA"] == 4 and sizes["collapse_publish"] == 0x40, sizes
    # a variable whose TYPE mentions sub0 is application state; a function or variable IN sub0 is retained code
    assert msvc_sub0_kind("?bus@?A0x1@@3V?$Slot@V?$Wiring@UA@@@sub0@@@collapse@@A", False) is None
    assert msvc_sub0_kind("?g_canceled@detail@sub0@@3_NA", False) == "sub0"
    assert msvc_sub0_kind("?f@?$Widget@H@sub0@@QEAAXXZ", True) == "sub0"
    assert msvc_sub0_kind("$unwind$?f@?$Widget@H@sub0@@QEAAXXZ", True) is None
    # a function whose template argument is &variable contains `@@3`; it is still retained sub0 code
    assert msvc_sub0_kind("??$publish@USample@?A0x1@@@?$StaticWiring@$1?relay@?A0x1@@3V?$Slot@URelay@?A0x1@@@"
                          "collapse@@A@sub0@@SAXAEBUSample@?A0x1@@@Z", True) == "sub0"
    assert info["functions"] == {"collapse_publish", "?deliver@detail@sub0@@YAXAEAUController@?A0x1@@AEBUSample@2@@Z",
                                 "memcpy", "?publish@?$Wiring@UA@@@sub0@@QEBAXAEBUE@@@Z"}, info["functions"]
    print("self-test passed")
    return 0


def sections(build_cfg, exe):
    if is_msvc(build_cfg):
        return msvc_section_sizes(parse_map(read_map(exe)))
    out = run([build_cfg["size"], exe]).stdout.splitlines()[1].split()
    text, data, bss = (int(x) for x in out[:3])
    init = 0
    for line in run([build_cfg["size"], "-A", exe]).stdout.splitlines():
        parts = line.split()
        if parts and parts[0] == ".init_array":
            init = int(parts[1])
    return {"text": text, "data": data, "bss": bss, "init_array": init}


def msvc_sub0_kind(name, is_function):
    """'sub0' or None for a decorated name, by what the symbol IS, as far as an ELF's demangled name says.
    Unwind records (`$unwind$f`) describe a function that is reported itself. A variable's decorated name embeds its
    TYPE (`?bus@?A0x1@@3V?$Slot@V?$Wiring@...@sub0@@@collapse@@A`), while an ELF variable name is only its own
    qualified name, so for a variable only the part before its `@@3` storage marker is searched. A function keeps the
    whole name, as the demangled ELF names do (template arguments included). Whether a symbol is a function comes
    from the map's `f` flag: a function name contains `@@3` too when a template argument is the address of a
    variable (`StaticWiring<&relay>`)."""
    if name.startswith("$"):
        return None
    head = name if is_function or "@@3" not in name else name.split("@@3", 1)[0] + "@@"
    return "sub0" if SUB0_NAMESPACE_MSVC.search(head) else None


def symbols_msvc(exe):
    """Symbol evidence from the linker map. Sizes are decorated-name keyed (undecorate() names the few reported)."""
    info = parse_map(read_map(exe))
    sizes = map_symbol_sizes(info)
    library = library_symbols(info)
    kinds = {n: msvc_sub0_kind(n, n in info["functions"]) for n in sizes if n not in library}
    retained = [(sizes[n], n) for n, k in kinds.items() if k]
    names = set(sizes)
    deps = [label for label, rx in DEPENDENCY_MARKERS_MSVC.items() if any(rx.match(n) for n in names)]
    shown = undecorate([n for _, n in retained])
    return {"retained_sub0_bytes": sum(sz for sz, _ in retained), "retained_sub0": shown,
            "dependencies": deps, "symbols": sizes}


def library_symbols(info):
    return {name for _, _, name, _, obj in info["symbols"] if ":" in obj}


def symbols(build_cfg, exe):
    if is_msvc(build_cfg):
        return symbols_msvc(exe)
    out = run([build_cfg["nm"], "-S", "--size-sort", exe]).stdout
    demangled = run([build_cfg["nm"], "-C", "-S", "--size-sort", exe]).stdout
    raw = [l.split()[-1] for l in out.splitlines() if l.split()]
    retained = []
    for line in demangled.splitlines():
        parts = line.split(None, 3)
        if len(parts) == 4 and SUB0_NAMESPACE.search(parts[3]):
            retained.append((int(parts[1], 16), parts[3]))
    sizes = {}
    for line in demangled.splitlines():
        parts = line.split(None, 3)
        if len(parts) == 4:
            sizes[parts[3]] = sizes.get(parts[3], 0) + int(parts[1], 16)
    undefined = run([build_cfg["nm"], "-u", exe]).stdout.split()
    names = set(raw) | set(undefined)
    deps = [label for label, rx in DEPENDENCY_MARKERS.items() if any(rx.match(n) for n in names)]
    return {"retained_sub0_bytes": sum(s for s, _ in retained), "retained_sub0": [n for _, n in retained],
            "dependencies": deps, "symbols": sizes}


def measure(build_name, build_cfg, case, variant, form, observable, out_dir):
    """A result dict, or {"error": why} for a failure."""
    exe, err = build(build_cfg, case, variant, observable, out_dir)
    if exe is None:
        return {"error": err}
    result = {"exe": exe}
    funcs, names = disassemble(build_cfg, exe)
    result["path"] = publish_path(build_cfg, funcs, names)
    if "collapse_publish" in result["path"]["external"]:
        return {"error": "collapse_publish not found in the disassembly"}
    result["sections"] = sections(build_cfg, exe)
    result.update(symbols(build_cfg, exe))
    if build_cfg["run"]:
        result["checksum"], err = checksum(exe)
        if err:
            return {"error": err}
        if shutil.which("valgrind"):
            result["instr"], err = callgrind(exe)
            if err:
                return {"error": err}
    return result


def verdicts(result, ref):
    """Criteria against the equal-work reference: True = meets, False = fails, None = not measured."""
    v = OrderedDict()
    if "checksum" in result:
        v["same behaviour"] = result["checksum"] == ref.get("checksum")
    if "instr" in result and "instr" in ref:
        for phase in ("publish", "setup", "teardown"):
            if phase in result["instr"]:
                tol = INSTR_TOLERANCE if phase == "publish" else 0
                v[f"{phase} instr"] = result["instr"][phase] <= ref["instr"][phase] + tol
    v["publish path"] = result["path"]["instructions"] <= ref["path"]["instructions"] + PATH_TOLERANCE
    v["no extra indirect calls"] = result["path"]["indirect_calls"] <= ref["path"]["indirect_calls"]
    v["no extra RAM"] = (result["sections"]["data"] + result["sections"]["bss"]) <= (ref["sections"]["data"] + ref["sections"]["bss"])
    v["no static init"] = result["sections"]["init_array"] <= ref["sections"]["init_array"]
    # Sub0Pub code that survives as a named out-of-line function (a Sink thunk) passes only
    # if the image is no larger than the reference's, i.e. it is the same code the reference has under another
    # name. A runtime registry's retained code always makes its image larger
    v["no Sub0Pub retained"] = result["retained_sub0_bytes"] == 0 or result["sections"]["text"] <= ref["sections"]["text"]
    extra = [d for d in result["dependencies"] if d not in ref["dependencies"]]
    v["no extra dependencies"] = not extra
    return v, extra


BUDGET_ALLOWANCE = {"publish": INSTR_TOLERANCE, "path": PATH_TOLERANCE}  # a passing criterion keeps its tolerance


def gated(variant):
    """Public-API variants carry regression budgets; the references do not."""
    return variant.startswith("sub0_") or variant.startswith("sub0pub_virtual")


def deltas(result, ref):
    """Metric deltas of a variant against its reference (the quantities the budgets bound)."""
    d = OrderedDict()
    for phase in ("publish", "setup", "teardown"):
        if phase in result.get("instr", {}) and phase in ref.get("instr", {}):
            d[phase] = round(result["instr"][phase] - ref["instr"][phase], 3)
    d["path"] = result["path"]["instructions"] - ref["path"]["instructions"]
    d["indirect_calls"] = result["path"]["indirect_calls"] - ref["path"]["indirect_calls"]
    d["text"] = result["sections"]["text"] - ref["sections"]["text"]
    d["ram"] = (result["sections"]["data"] + result["sections"]["bss"]) - (ref["sections"]["data"] + ref["sections"]["bss"])
    d["init_array"] = result["sections"]["init_array"] - ref["sections"]["init_array"]
    return d


def budget_key(build_name, case, form, variant):
    return f"{build_name}/{case}/{form}/{variant}"


def make_budget(result, ref):
    b = OrderedDict((k, max(v, BUDGET_ALLOWANCE.get(k, 0))) for k, v in deltas(result, ref).items())
    b["added_deps"] = sorted(d for d in result["dependencies"] if d not in ref["dependencies"])
    return b


def check_budget(result, ref, budget):
    """Breaches of a budget: a list of 'metric delta > budget' strings."""
    over = []
    measured = deltas(result, ref)
    # A missing profiler result must not silently disable recorded instruction limits. Conversely, every
    # measured metric needs an explicit limit: a truncated budget must not turn a regression into a pass.
    for k in budget:
        if k != "added_deps" and k not in measured:
            over.append(f"{k}: missing measurement")
    for k, v in measured.items():
        if k not in budget:
            over.append(f"{k}: missing budget")
        elif v > budget[k] + 1e-6:
            over.append(f"{k} {v:+g} > {budget[k]:+g}")
    if "added_deps" not in budget:
        over.append("added_deps: missing budget")
    extra = sorted(set(d for d in result["dependencies"] if d not in ref["dependencies"]) - set(budget.get("added_deps", [])))
    if extra:
        over.append("added deps " + ", ".join(extra))
    return over


def fmt_delta(value, ref, digits=0):
    d = value - ref
    return f"{value:.{digits}f} ({d:+.{digits}f})"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--case")
    ap.add_argument("--profile", choices=("full", "smoke"), default="full",
                    help="case set to measure; smoke names come from smoke_cases.txt")
    ap.add_argument("--build", action="append", help="named build to measure (repeat to select several)")
    ap.add_argument("--json")
    ap.add_argument("--budgets", help="fail if a public-API variant exceeds its recorded budget (regression gate)")
    ap.add_argument("--write-budgets", help="record the measured deltas of the public-API variants as budgets")
    ap.add_argument("--self-test", action="store_true", help="check the publish-path parser and exit")
    args = ap.parse_args()
    if args.self_test:
        return self_test()

    builds = available_builds()
    if args.build:
        missing = sorted(set(args.build) - builds.keys())
        if missing:
            print(f"**Build unavailable:** {', '.join(missing)}", file=sys.stderr)
            return 2
        builds = OrderedDict((n, b) for n, b in builds.items() if n in args.build)
    if not builds:
        print("**No build selected:** no toolchain available", file=sys.stderr)
        return 2
    try:
        cases = select_cases(args.profile, args.case)
    except ValueError as error:
        print(f"**Invalid case profile:** {error}", file=sys.stderr)
        return 2
    if not cases:
        print(f"**No case selected:** {args.case}", file=sys.stderr)
        return 2
    # A variant naming a reference the case does not have fails (it would silently fall back otherwise)
    failures = []
    for case, variants in cases.items():
        for variant in variants:
            ref_name = variant_reference(case, variant)
            if ref_name and ref_name not in variants:
                failures.append(f"{case}/{variant}: reference {ref_name} does not exist")

    results = OrderedDict()
    broken = []
    with tempfile.TemporaryDirectory() as tmp:
        for build_name, build_cfg in builds.items():
            for case, variants in cases.items():
                if build_cfg.get("multi_tu_only") and not multi_tu(case, variants):
                    continue
                for form, observable in FORMS.items():
                    for variant in variants:
                        key = (build_name, case, form, variant)
                        results[key] = r = measure(build_name, build_cfg, case, variant, form, observable, tmp)
                        if "error" in r:
                            failures.append(f"{'/'.join(key)}: {r['error']}")
            ran = [k for k in results if k[0] == build_name]
            if ran and all("error" in results[k] for k in ran):
                failures.append(f"{build_name}: nothing measured")
    if not results:
        failures.append("the case/build selection measured nothing (e.g. an LTO build with a single-TU case)")

    print("# Collapse evidence\n")
    print("Final-link evidence per case, build and form; every variant is compared with `handwritten` "
          "(equal-work reference, same build and form), or with the extra reference it names, shown as "
          "`variant (vs handwritten_<kind>)`: e.g. `handwritten_runtime`, hand-written code that reaches its "
          "receivers through addresses stored at setup. Deltas in parentheses. "
          f"instr = callgrind instructions (publish: per publication of {PUBLISHES}). "
          "path = static instructions of `collapse_publish` plus directly reachable functions.\n")
    for build_name, build_cfg in builds.items():
        ver = compiler_version(build_cfg)
        print(f"- **{build_name}**: `{ver}` `{' '.join(build_cfg['flags'])}`")
    print()

    gate_rows = OrderedDict()
    for case, variants in cases.items():
        print(f"## Case: {case}\n")
        for build_name, build_cfg in builds.items():
            if (build_name, case, "observable", REFERENCE) not in results:
                continue
            for form in FORMS:
                ref = results[(build_name, case, form, REFERENCE)]
                print(f"### {build_name}, {form} work\n")
                if "error" in ref:
                    print(f"handwritten failed: `{ref['error']}`\n")
                    continue
                cols = ["variant", "checksum", "publish instr", "setup instr", "teardown instr", "path instr",
                        "calls (direct/indirect)", "text", "data+bss", "retained sub0 (B)", "added deps", "verdict"]
                print("| " + " | ".join(cols) + " |")
                print("|" + "---|" * len(cols))
                for variant in variants:
                    r = results[(build_name, case, form, variant)]
                    if "error" in r:
                        print(f"| {variant} | **FAILED**: `{r['error']}` |" + " |" * (len(cols) - 2))
                        continue
                    ref_name = variant_reference(case, variant)
                    ref = results.get((build_name, case, form, ref_name or REFERENCE), {"error": "missing"})
                    if "error" in ref:
                        print(f"| {variant} | reference {ref_name} unavailable |" + " |" * (len(cols) - 2))
                        continue
                    shown = variant if ref_name in (None, REFERENCE) else f"{variant} (vs {ref_name})"
                    v, extra = verdicts(r, ref) if variant != REFERENCE else (OrderedDict(), [])
                    if variant != REFERENCE and v.get("same behaviour") is False:
                        broken.append((build_name, case, form, variant))
                    if gated(variant):
                        gate_rows[budget_key(build_name, case, form, variant)] = (r, ref)
                    ins = r.get("instr", {})
                    rins = ref.get("instr", {})
                    def phase(p, digits):
                        return fmt_delta(ins[p], rins[p], digits) if p in ins and p in rins else "-"
                    ram = r["sections"]["data"] + r["sections"]["bss"]
                    rram = ref["sections"]["data"] + ref["sections"]["bss"]
                    failed = [k for k, ok in v.items() if ok is False]
                    verdict = "reference" if variant == REFERENCE else ("PASS" if not failed else "FAIL: " + ", ".join(failed))
                    if variant.startswith(REFERENCE + "_"):
                        verdict = "reference; " + verdict
                    same = "-" if "checksum" not in r else ("ok" if variant == REFERENCE or r["checksum"] == ref.get("checksum") else "**DIFFERS**")
                    print("| " + " | ".join([
                        shown, same, phase("publish", 1), phase("setup", 0), phase("teardown", 0),
                        fmt_delta(r["path"]["instructions"], ref["path"]["instructions"]),
                        f"{r['path']['direct_calls']}/{r['path']['indirect_calls']}",
                        fmt_delta(r["sections"]["text"], ref["sections"]["text"]),
                        fmt_delta(ram, rram),
                        str(r["retained_sub0_bytes"]),
                        ", ".join(extra) or "-",
                        verdict]) + " |")
                print()
            # Where the extra image comes from: largest symbols a variant adds over the reference (observable form)
            for variant in variants[1:]:
                ref = results.get((build_name, case, "observable", variant_reference(case, variant)), {"error": "-"})
                r = results[(build_name, case, "observable", variant)]
                if "error" in r or "error" in ref:
                    continue
                added = sorted(((sz, n) for n, sz in r["symbols"].items()
                                if n not in ref["symbols"] or sz > ref["symbols"][n]), reverse=True)[:8]
                if added:
                    print(f"<details><summary>{build_name}: largest symbols added by {variant} (bytes)</summary>\n")
                    for sz, n in zip((sz for sz, _ in added), undecorate([n for _, n in added])):
                        print(f"- {sz} `{n}`")
                    print("\n</details>\n")

    if args.json:
        serial = []
        for (b, c, f, v), r in results.items():
            r = {k: val for k, val in r.items() if k not in ("exe", "symbols")}
            serial.append({"build": b, "case": c, "form": f, "variant": v, **r})
        with open(args.json, "w") as fh:
            json.dump(serial, fh, indent=1)

    toolchains = OrderedDict((n, compiler_version(b)) for n, b in builds.items())
    if args.write_budgets:
        recorded = {"budgets": OrderedDict()}
        if os.path.exists(args.write_budgets):  # a partial selection updates only the rows it measured
            with open(args.write_budgets) as fh:
                recorded = json.load(fh, object_pairs_hook=OrderedDict)
        recorded.setdefault("toolchains", OrderedDict()).update(toolchains)
        for key, (r, ref) in gate_rows.items():
            recorded["budgets"][key] = make_budget(r, ref)
        recorded["budgets"] = OrderedDict(sorted(recorded["budgets"].items()))
        with open(args.write_budgets, "w") as fh:
            json.dump(recorded, fh, indent=1)
            fh.write("\n")
    breaches = []
    if args.budgets:
        with open(args.budgets) as fh:
            recorded = json.load(fh)
        for name, ver in toolchains.items():
            if recorded.get("toolchains", {}).get(name) not in (None, ver):
                print(f"Note: {name} is `{ver}`; budgets were recorded with `{recorded['toolchains'][name]}`.\n")
        for key, (r, ref) in gate_rows.items():
            budget = recorded["budgets"].get(key)
            if budget is None:
                breaches.append(f"{key}: no budget (record one with --write-budgets)")
                continue
            over = check_budget(r, ref, budget)
            if over:
                breaches.append(f"{key}: " + "; ".join(over))
        print(f"**Regression gate:** {len(gate_rows)} public-API measurements against `{args.budgets}`: "
              + (f"{len(breaches)} over budget\n" if breaches else "all within budget\n"))
        for b in breaches:
            print(f"- {b}")
        if breaches:
            print()

    # Gating: behaviour mismatches, build/run/measurement failures, missing references and (with --budgets)
    # public-API regressions fail the tool. Cost verdicts stay report-only: they compare with hand-written code,
    # the budgets with the recorded state.
    status = 0
    if broken:
        print("**Behaviour mismatch:** " + ", ".join("/".join(k) for k in broken) + "\n")
        status = 1
    if breaches:
        print(f"{len(breaches)} budget breach(es)", file=sys.stderr)
        status = 1
    if failures:
        print("**Failed:** " + "; ".join(failures) + "\n")
        print(f"{len(failures)} failure(s)", file=sys.stderr)
        status = 1
    return status


if __name__ == "__main__":
    sys.exit(main())
