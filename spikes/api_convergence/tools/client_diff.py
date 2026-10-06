#!/usr/bin/env python3
"""Client-code cost of switching delivery mode, per candidate API (see ../WORKFLOW.md).

Every candidate under ../examples is one weather station built in several modes. Sources outside `mode/` are shared
by all of its builds; `mode/<mode>/` holds what that mode alone contributes. The cost of switching is therefore the
difference between `mode/dynamic/` and `mode/<other>/`, counted in lines of code (blank lines, comments and
`#pragma once` are ignored). A candidate converges the API to the extent that this difference is small and stays
out of the participants (the publisher and receiver classes).

Usage: python tools/client_diff.py [--baseline dynamic] > client_diff.md
"""
import argparse
import difflib
import os
import sys

SPIKE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXAMPLES = os.path.join(SPIKE, "examples")
SOURCE_SUFFIXES = (".hpp", ".cpp")
PARTICIPANT_FILES = {"participants.hpp"}  # a mode-specific copy of the publisher and receiver classes


def code_lines(path):
    """The lines of a source file that are code: no blanks, comments or `#pragma once`."""
    lines, in_block = [], False
    with open(path, encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if in_block:
                in_block = "*/" not in line
                continue
            if line.startswith("/*"):
                in_block = "*/" not in line
                continue
            if not line or line.startswith("//") or line == "#pragma once":
                continue
            lines.append(line.split(" // ")[0].rstrip())
    return lines


def mode_files(candidate, mode):
    directory = os.path.join(EXAMPLES, candidate, "mode", mode)
    return {name: code_lines(os.path.join(directory, name))
            for name in sorted(os.listdir(directory)) if name.endswith(SOURCE_SUFFIXES)}


def shared_line_count(candidate):
    """Code lines of the candidate that every mode shares (everything outside mode/)."""
    total = 0
    root = os.path.join(EXAMPLES, candidate)
    for directory, subdirectories, names in os.walk(root):
        if os.path.basename(directory) == "mode":
            subdirectories.clear()
            continue
        total += sum(len(code_lines(os.path.join(directory, name))) for name in names if name.endswith(SOURCE_SUFFIXES))
    return total


def changed(before, after):
    """(removed, added) code lines between two versions of a file."""
    removed = added = 0
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, before, after, autojunk=False).get_opcodes():
        if tag != "equal":
            removed += i2 - i1
            added += j2 - j1
    return removed, added


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--baseline", default="dynamic", help="the mode every other mode is compared with")
    args = parser.parse_args()

    candidates = sorted(name for name in os.listdir(EXAMPLES) if os.path.isdir(os.path.join(EXAMPLES, name, "mode")))
    print("# Client-code cost of switching delivery mode\n")
    print(f"Lines of code that differ between a candidate's `{args.baseline}` build and each other build of the same "
          "station (`tools/client_diff.py`). Shared lines are the sources every build of that candidate compiles "
          "unchanged. A participant edit means the publisher or receiver classes themselves had to change.\n")
    print("| candidate | switch | lines removed | lines added | files touched | participant lines edited | shared lines |")
    print("|---|---|---:|---:|---|---:|---:|")
    for candidate in candidates:
        modes = sorted(os.listdir(os.path.join(EXAMPLES, candidate, "mode")))
        if args.baseline not in modes:
            sys.exit(f"{candidate}: no `{args.baseline}` mode")
        baseline = mode_files(candidate, args.baseline)
        shared = shared_line_count(candidate)
        for mode in (m for m in modes if m != args.baseline):
            other = mode_files(candidate, mode)
            removed = added = participant = 0
            touched = []
            for name in sorted(set(baseline) | set(other)):
                file_removed, file_added = changed(baseline.get(name, []), other.get(name, []))
                if file_removed or file_added:
                    touched.append(name)
                    removed += file_removed
                    added += file_added
                    if name in PARTICIPANT_FILES:
                        participant += file_removed + file_added
            print(f"| {candidate} | {args.baseline} -> {mode} | {removed} | {added} | {', '.join(touched) or '-'} | "
                  f"{participant} | {shared} |")
    return 0


if __name__ == "__main__":
    sys.exit(main())
