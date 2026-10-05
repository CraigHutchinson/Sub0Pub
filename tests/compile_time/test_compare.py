"""Harness contracts: failures must not become timings and sample order/statistics remain inspectable."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("compile_compare", Path(__file__).with_name("compare.py"))
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)


class CompileTimeContracts(unittest.TestCase):
    def test_sample_summary_reports_spread_without_outlier_removal(self):
        stats = compare.summary([1, 2, 3, 4, 100])
        self.assertEqual(stats, {"median_s": 3, "min_s": 1, "max_s": 100, "mad_s": 1})

    def test_alternating_order_balances_each_two_rounds(self):
        self.assertEqual([compare.lane_order(i) for i in range(4)],
                         [("A", "B"), ("B", "A"), ("A", "B"), ("B", "A")])

    def test_capture_excludes_warmups_and_preserves_lane_identity_when_order_reverses(self):
        args = argparse.Namespace(compiler=sys.executable, baseline="base", candidate="head",
                                  baseline_standard="c++17", candidate_standard="c++23", translation_units=1,
                                  types=1, receivers=1, samples=3, warmups=1, profiles=["wiring"])
        # Warmup A/B; measured B/A, A/B, B/A. A must be [1, 2, 3], B must be [4, 5, 6].
        with patch.object(compare, "revision", side_effect=["base-sha", "head-sha"]), \
             patch.object(compare, "snapshot"), \
             patch.object(compare, "measure", side_effect=[100, 200, 4, 1, 2, 5, 6, 3]):
            row = compare.run(args)["results"][0]
        self.assertEqual([pair["seconds"]["A"] for pair in row["samples"]], [1, 2, 3])
        self.assertEqual([pair["seconds"]["B"] for pair in row["samples"]], [4, 5, 6])
        self.assertEqual(row["median_change_percent"], 150)

    def test_failed_compilation_is_not_a_sample_and_cleans_objects(self):
        with tempfile.TemporaryDirectory() as directory:
            objects = Path(directory) / "objects"
            with patch.object(compare, "command", side_effect=subprocess.CalledProcessError(1, "compiler")):
                with self.assertRaises(subprocess.CalledProcessError):
                    compare.measure("compiler", [], Path(directory), [Path("input.cpp")], objects, 0,
                                    argparse.Namespace(types=1, receivers=1), {})
            self.assertFalse(objects.exists())

    def test_success_without_objects_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            objects = Path(directory) / "objects"
            with patch.object(compare, "command"):
                with self.assertRaisesRegex(ValueError, "without producing"):
                    compare.measure("compiler", [], Path(directory), [Path("input.cpp")], objects, 0,
                                    argparse.Namespace(types=1, receivers=1), {})
            self.assertFalse(objects.exists())

    def test_missing_revision_cannot_produce_a_report(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "result.json"
            run = subprocess.run([sys.executable, str(Path(compare.__file__)), "--baseline", "not-a-real-ref",
                                  "--compiler", sys.executable, "--json", str(report),
                                  "--markdown", str(Path(directory) / "result.md")], capture_output=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertFalse(report.exists())


if __name__ == "__main__":
    unittest.main()
