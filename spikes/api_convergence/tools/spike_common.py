"""Shared setup for the spike tools: the stock collapse-evidence module, pointed at the spike cases and driver."""
import os
import sys

SPIKE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.normpath(os.path.join(SPIKE, "..", ".."))
COLLAPSE = os.path.join(ROOT, "tests", "collapse")
CASES = os.path.join(SPIKE, "cases")
INCLUDES = [COLLAPSE, os.path.join(ROOT, "include"), os.path.join(SPIKE, "include")]

# Where the optional Windows toolchains live when they are installed but not on PATH
TOOL_DIRS = [r"C:\Program Files\LLVM\bin",
             r"C:\Program Files (x86)\Intel\oneAPI\compiler\latest\bin",
             r"C:\Program Files (x86)\Intel\oneAPI\vtune\latest\bin64"]
INTEL_LIB = r"C:\Program Files (x86)\Intel\oneAPI\compiler\latest\lib"

sys.path.insert(0, COLLAPSE)
import collapse_evidence as ce  # noqa: E402

_configured = False


def collapse_evidence():
    """The stock tool's module, configured once for the spike: cases, driver, include paths, tool environment."""
    global _configured
    if _configured:
        return ce
    _configured = True
    ce.CASES_DIR = CASES
    ce.HERE = os.path.join(SPIKE, "harness")  # driver.cpp; collapse_case.hpp is reached through the include path
    extra = [COLLAPSE, os.path.join(SPIKE, "include")]
    ce.MSVC_COMMON.extend("/I" + path for path in extra)
    ce.COMMON.extend("-I" + path for path in extra)
    if sys.platform == "win32":
        ce.ensure_msvc_environment()
        for directory in TOOL_DIRS:
            if os.path.isdir(directory) and directory.lower() not in os.environ["PATH"].lower():
                os.environ["PATH"] += os.pathsep + directory
        if os.path.isdir(INTEL_LIB):
            os.environ["LIB"] = os.environ.get("LIB", "") + os.pathsep + INTEL_LIB
        base = ce.BUILDS["msvc-O2"]
        ce.BUILDS["clangcl-O2"] = dict(base, cxx="clang-cl", version_command=["clang-cl", "--version"])
        ce.BUILDS["icx-O2"] = dict(base, cxx="icx-cl", version_command=["icx-cl", "--version"])
    return ce


def first_line(command):
    """The first non-empty line a command prints (a compiler's version banner)."""
    result = ce.run(command)
    return next((line.strip() for line in (result.stdout + result.stderr).splitlines() if line.strip()), command[0])
