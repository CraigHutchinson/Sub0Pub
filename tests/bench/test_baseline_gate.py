"""The release benchmark gate fails on regressions, missing measurements, and malformed budgets."""
import importlib.util
import json
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("baseline", HERE / "run_baseline.py")
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


class BaselineGateTests(unittest.TestCase):
    def test_recorded_budgets_are_unique_and_cover_core_paths(self):
        budgets = json.loads((HERE / "budgets.json").read_text())
        measured = {
            (row["suite"], row["section"], row["scenario"], row["variant"]): row["max_instr_per_op"]
            for row in budgets["measurements"]
        }
        self.assertEqual(len(measured), len(budgets["measurements"]))
        self.assertEqual(baseline.check_budgets(measured, budgets), [])
        self.assertEqual({key[0] for key in measured}, {"Core publish/subscribe by policy", "IPC end-to-end"})

    def test_regression_and_missing_measurement_fail(self):
        key = ("Core publish/subscribe by policy", "Publish", "1 subscriber", "Direct (default)")
        budgets = {"measurements": [dict(zip(("suite", "section", "scenario", "variant"), key),
                                          max_instr_per_op=38.0)]}
        self.assertIn("39.0 > 38.0", baseline.check_budgets({key: 39.0}, budgets)[0])
        self.assertIn("missing measurement", baseline.check_budgets({}, budgets)[0])
        self.assertEqual(baseline.check_budgets({key: 38.0001}, budgets), [])
        self.assertIn("38.1 > 38.0", baseline.check_budgets({key: 38.051}, budgets)[0])

    def test_duplicate_and_empty_budgets_fail(self):
        key = ("Core publish/subscribe by policy", "Publish", "1 subscriber", "Direct (default)")
        row = dict(zip(("suite", "section", "scenario", "variant"), key), max_instr_per_op=38.0)
        self.assertIn("duplicate budget", baseline.check_budgets({key: 38.0}, {"measurements": [row, row]})[0])
        self.assertEqual(baseline.check_budgets({}, {"measurements": []}), ["no benchmark budgets selected"])


if __name__ == "__main__":
    unittest.main()
