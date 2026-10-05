"""Evidence failures must fail the job; no external compiler/profiler needed."""
import copy
import json
import tempfile
import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("evidence", Path(__file__).resolve().parents[1] / "collapse_evidence.py")
evidence = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evidence)


class EvidenceGates(unittest.TestCase):
    def test_smoke_profile_is_a_real_subset_with_cross_file_lto(self):
        all_cases = evidence.select_cases("full")
        smoke_cases = evidence.select_cases("smoke")
        self.assertGreater(len(all_cases), len(smoke_cases))
        self.assertIn("cross_file", smoke_cases)
        self.assertTrue(all("handwritten" in variants for variants in smoke_cases.values()))

    def test_msvc_environment_accepts_mixed_case_path(self):
        responses = [
            subprocess.CompletedProcess([], 0, "C:/Visual Studio\n", ""),
            subprocess.CompletedProcess([], 0, "Path=C:/VC/bin\nInclude=C:/VC/include\n", ""),
        ]
        with patch.object(evidence.sys, "platform", "win32"), \
             patch.object(evidence.shutil, "which", side_effect=[None, "C:/VC/bin/cl.exe"]), \
             patch.object(evidence.os.path, "isfile", return_value=True), \
             patch.dict(evidence.os.environ, {}, clear=True), \
             patch.object(evidence, "run", side_effect=responses):
            evidence.ensure_msvc_environment()
            self.assertEqual(evidence.os.environ.get("PATH"), "C:/VC/bin")
            self.assertEqual(evidence.os.environ.get("INCLUDE"), "C:/VC/include")

    def test_msvc_build_reports_compiler_diagnostic(self):
        output = "compiler banner\nsource.cpp\nsource.cpp(1): fatal error C1083: missing header\n"
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(evidence, "run", return_value=subprocess.CompletedProcess([], 2, output, "")):
            exe, error = evidence.build_msvc(evidence.BUILDS["msvc-O2"], "one_receiver", "handwritten", 1, tmp)
        self.assertIsNone(exe)
        self.assertEqual(error, "source.cpp(1): fatal error C1083: missing header")

    def budget_run(self, change_result=None, change_budget=None):
        reference = {
            "checksum": "same", "instr": {"publish": 10, "setup": 10, "teardown": 10},
            "path": {"instructions": 10, "direct_calls": 0, "indirect_calls": 0},
            "sections": {"text": 100, "data": 0, "bss": 0, "init_array": 0},
            "retained_sub0_bytes": 0,
            "dependencies": [], "symbols": {},
        }
        result = copy.deepcopy(reference)
        budget = evidence.make_budget(result, reference)
        if change_result:
            change_result(result)
        if change_budget:
            change_budget(budget)
        rows = {evidence.budget_key("gcc-O2", "one_receiver", form, "sub0_b2_static"): budget
                for form in evidence.FORMS}
        def measure(build, config, case, variant, form, observable, tmp):
            return copy.deepcopy(reference if variant == "handwritten" else result)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "budgets.json"
            path.write_text(json.dumps({"budgets": rows}))
            output = io.StringIO()
            with patch.object(sys, "argv", ["evidence", "--budgets", str(path)]), \
                 patch.object(evidence, "available_builds", return_value={"gcc-O2": evidence.BUILDS["gcc-O2"]}), \
                 patch.object(evidence, "discover_cases", return_value={"one_receiver": ["handwritten", "sub0_b2_static"]}), \
                 patch.object(evidence, "measure", side_effect=measure), \
                 patch.object(evidence, "run", return_value=subprocess.CompletedProcess([], 0, "test compiler\n", "")), \
                 contextlib.redirect_stdout(output), contextlib.redirect_stderr(io.StringIO()):
                return evidence.main(), output.getvalue()

    def test_complete_budget_passes(self):
        status, _ = self.budget_run()
        self.assertEqual(status, 0)

    def test_cost_regression_fails_main(self):
        def regress(result):
            result["instr"]["publish"] += 100
            result["sections"]["bss"] += 4096
        status, output = self.budget_run(change_result=regress)
        self.assertEqual(status, 1)
        self.assertIn("publish +100", output)
        self.assertIn("ram +4096", output)

    def test_missing_profiler_measurements_fail_main(self):
        status, output = self.budget_run(change_result=lambda result: result.pop("instr"))
        self.assertEqual(status, 1)
        self.assertIn("publish: missing measurement", output)

    def test_missing_metric_budget_fails_main(self):
        status, output = self.budget_run(change_budget=lambda budget: budget.pop("ram"))
        self.assertEqual(status, 1)
        self.assertIn("ram: missing budget", output)

    def test_missing_dependency_budget_fails_main(self):
        status, output = self.budget_run(change_budget=lambda budget: budget.pop("added_deps"))
        self.assertEqual(status, 1)
        self.assertIn("added_deps: missing budget", output)

    def test_broken_reference_is_not_success(self):
        with patch.object(sys, "argv", ["evidence"]), \
             patch.object(evidence, "available_builds", return_value={"gcc-O2": evidence.BUILDS["gcc-O2"]}), \
             patch.object(evidence, "discover_cases", return_value={"one_receiver": ["handwritten"]}), \
             patch.object(evidence, "measure", return_value={"error": "injected compile error"}), \
             patch.object(evidence, "run", return_value=subprocess.CompletedProcess([], 0, "test compiler\n", "")), \
             contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(evidence.main(), 1)

    def test_empty_selection_is_not_success(self):
        with patch.object(sys, "argv", ["evidence"]), \
             patch.object(evidence, "available_builds", return_value={}), \
             contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(evidence.main(), 2)

    def test_repeated_build_selection_measures_both_builds(self):
        builds = {name: evidence.BUILDS[name] for name in ("gcc-O2", "gcc-O2-lto")}
        with patch.object(sys, "argv", ["evidence", "--build", "gcc-O2", "--build", "gcc-O2-lto"]), \
             patch.object(evidence, "available_builds", return_value=builds), \
             patch.object(evidence, "discover_cases", return_value={"cross_file": ["handwritten"]}), \
             patch.object(evidence, "measure", return_value={"error": "injected compile error"}) as measure, \
             patch.object(evidence, "run", return_value=subprocess.CompletedProcess([], 0, "test compiler\n", "")), \
             contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(evidence.main(), 1)
        self.assertEqual({call.args[0] for call in measure.call_args_list}, set(builds))

    def test_crash_after_checksum_is_not_success(self):
        with patch.object(evidence, "run", return_value=subprocess.CompletedProcess([], 1, "checksum 42\n", "failed")):
            checksum, error = evidence.checksum("unused")
            self.assertIsNone(checksum)
            self.assertIsNotNone(error)

    def test_profiler_failure_with_complete_dumps_is_not_success(self):
        def failed_profiler(command):
            output = next(arg.split("=", 1)[1] for arg in command if arg.startswith("--callgrind-out-file="))
            for phase in ("setup", "publish", "teardown"):
                Path(output.replace("%p", phase)).write_text(
                    f"desc: Trigger: Client Request: {phase}\nsummary: 1000\n")
            return subprocess.CompletedProcess(command, 1, "", "injected profiler failure")
        with patch.object(evidence, "run", side_effect=failed_profiler):
            _, error = evidence.callgrind("unused")
            self.assertIsNotNone(error)
            self.assertIn("exit=1", error)


if __name__ == "__main__":
    unittest.main()
