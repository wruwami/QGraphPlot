"""Structural validation tests for .github/workflows/ci.yml.

This project's C++ code is covered by QtTest (see tests/Test*.cpp), but the
CI pipeline definition is plain YAML with no application logic to exercise
via QtTest. These tests parse the workflow file and assert on its structure
(triggers, jobs, permissions, matrix, steps, and conditionals) so that
accidental regressions to the CI configuration are caught automatically.

Run with:
    python3 -m unittest tests/test_ci_workflow.py -v
or:
    python3 -m pytest tests/test_ci_workflow.py -v

Requires PyYAML (``pip install pyyaml``).
"""

import pathlib
import unittest

import yaml

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "ci.yml"


def load_workflow():
    with open(WORKFLOW_PATH, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def get_on_triggers(data):
    # PyYAML (YAML 1.1) resolves the bare scalar key "on" to the boolean
    # True, so GitHub Actions' "on:" trigger block is not reachable via the
    # string key "on" -- it must be looked up via the boolean key instead.
    return data.get("on", data.get(True))


def get_step(job, name):
    for step in job.get("steps", []):
        if step.get("name") == name:
            return step
    return None


class CIWorkflowStructureTests(unittest.TestCase):
    """Top-level workflow structure: name, triggers, permissions, jobs."""

    @classmethod
    def setUpClass(cls):
        cls.data = load_workflow()

    def test_workflow_file_exists(self):
        self.assertTrue(WORKFLOW_PATH.is_file())

    def test_workflow_parses_as_mapping(self):
        self.assertIsInstance(self.data, dict)

    def test_workflow_name_is_ci(self):
        self.assertEqual(self.data.get("name"), "CI")

    def test_top_level_keys_are_minimal(self):
        # Regression guard: the old workflow carried extra top-level keys
        # (concurrency, env with QT_VERSION/CMAKE_VERSION) that were
        # intentionally dropped in this revision.
        self.assertEqual(
            set(self.data.keys()), {"name", True, "permissions", "jobs"}
        )

    def test_triggers_on_push_to_main(self):
        on = get_on_triggers(self.data)
        self.assertIn("push", on)
        self.assertEqual(on["push"], {"branches": ["main"]})

    def test_triggers_on_pull_request_without_branch_filter(self):
        on = get_on_triggers(self.data)
        self.assertIn("pull_request", on)
        # No "branches:" restriction anymore -> PRs targeting any branch
        # trigger CI (this is a change from the previous main-only filter).
        self.assertIsNone(on["pull_request"])

    def test_top_level_permissions_are_read_only(self):
        self.assertEqual(self.data.get("permissions"), {"contents": "read"})

    def test_jobs_are_exactly_lint_and_build_and_test(self):
        # Regression guard: the previous workflow had four jobs
        # (build-and-test, static-analysis, format-check) chained together;
        # this revision consolidates static analysis/formatting into "lint"
        # and decouples it from build-and-test.
        self.assertEqual(set(self.data["jobs"].keys()), {"lint", "build-and-test"})

    def test_no_self_hosted_runners_referenced(self):
        raw = WORKFLOW_PATH.read_text(encoding="utf-8")
        self.assertNotIn("self-hosted", raw)

    def test_jobs_have_no_needs_dependency(self):
        # lint and build-and-test must run independently/in parallel.
        for job_name, job in self.data["jobs"].items():
            self.assertNotIn("needs", job, f"job '{job_name}' should not declare needs")

    def test_all_steps_have_names(self):
        for job_name, job in self.data["jobs"].items():
            for i, step in enumerate(job.get("steps", [])):
                self.assertTrue(
                    step.get("name"),
                    f"step #{i} in job '{job_name}' is missing a name",
                )


class LintJobTests(unittest.TestCase):
    """The 'lint' job: clang-format auto-fix + cppcheck + SARIF upload."""

    @classmethod
    def setUpClass(cls):
        cls.job = load_workflow()["jobs"]["lint"]

    def test_runs_on_ubuntu_latest(self):
        self.assertEqual(self.job["runs-on"], "ubuntu-latest")

    def test_elevated_permissions_for_autocommit_and_sarif(self):
        self.assertEqual(
            self.job.get("permissions"),
            {"contents": "write", "security-events": "write"},
        )

    def test_step_order(self):
        names = [s.get("name") for s in self.job["steps"]]
        self.assertEqual(
            names,
            [
                "Checkout",
                "Install clang-format and cppcheck",
                "clang-format (auto-fix and commit)",
                "cppcheck",
                "Convert Cppcheck XML to SARIF",
                "Upload SARIF to GitHub",
            ],
        )

    def test_checkout_uses_head_ref_for_pr_branch(self):
        step = get_step(self.job, "Checkout")
        self.assertEqual(step["uses"], "actions/checkout@v4")
        self.assertEqual(
            step.get("with", {}).get("ref"),
            "${{ github.head_ref || github.ref_name }}",
        )

    def test_install_step_installs_clang_format_and_cppcheck(self):
        step = get_step(self.job, "Install clang-format and cppcheck")
        run = step.get("run", "")
        self.assertIn("clang-format", run)
        self.assertIn("cppcheck", run)

    def test_clang_format_autofix_targets_expected_directories(self):
        step = get_step(self.job, "clang-format (auto-fix and commit)")
        run = step.get("run", "")
        self.assertIn("find src tests examples", run)
        self.assertIn("clang-format -i", run)

    def test_clang_format_autofix_commits_and_pushes_to_target_branch(self):
        step = get_step(self.job, "clang-format (auto-fix and commit)")
        self.assertEqual(
            step.get("env", {}).get("TARGET_BRANCH"),
            "${{ github.head_ref || github.ref_name }}",
        )
        run = step.get("run", "")
        self.assertIn("git commit -m", run)
        self.assertIn('git push origin "HEAD:$TARGET_BRANCH"', run)

    def test_clang_format_autofix_only_pushes_when_files_changed(self):
        step = get_step(self.job, "clang-format (auto-fix and commit)")
        run = step.get("run", "")
        self.assertIn("git diff --quiet", run)

    def test_cppcheck_step_uses_expected_flags(self):
        step = get_step(self.job, "cppcheck")
        run = step.get("run", "")
        self.assertIn("--std=c++17", run)
        self.assertIn("--language=c++", run)
        self.assertIn("--enable=warning,style,performance,portability", run)
        self.assertIn("--suppressions-list=.cppcheck-suppressions", run)
        self.assertIn("--xml --xml-version=2", run)
        self.assertIn("-I src/core", run)
        self.assertIn("src tests", run)
        self.assertIn("2> cppcheck.xml", run)

    def test_cppcheck_step_does_not_use_error_exitcode(self):
        # Unlike cppcheck.options, this CI step relies on the SARIF upload
        # for reporting rather than failing the build directly.
        step = get_step(self.job, "cppcheck")
        run = step.get("run", "")
        self.assertNotIn("--error-exitcode", run)

    def test_sarif_conversion_step(self):
        step = get_step(self.job, "Convert Cppcheck XML to SARIF")
        self.assertEqual(step["uses"], "Flast/cppcheck-sarif@v2")
        self.assertEqual(
            step.get("with"), {"input": "cppcheck.xml", "output": "cppcheck.sarif"}
        )

    def test_sarif_upload_step(self):
        step = get_step(self.job, "Upload SARIF to GitHub")
        self.assertEqual(step["uses"], "github/codeql-action/upload-sarif@v3")
        self.assertEqual(step.get("with"), {"sarif_file": "cppcheck.sarif"})


class BuildAndTestJobTests(unittest.TestCase):
    """The 'build-and-test' job: cross-platform matrix build/test/coverage."""

    @classmethod
    def setUpClass(cls):
        cls.job = load_workflow()["jobs"]["build-and-test"]

    def test_runs_on_matrix_os(self):
        self.assertEqual(self.job["runs-on"], "${{ matrix.os }}")

    def test_name_includes_os_and_qt_version(self):
        self.assertEqual(
            self.job["name"], "${{ matrix.os }} / Qt ${{ matrix.qt-version }}"
        )

    def test_strategy_fail_fast_disabled(self):
        self.assertIs(self.job["strategy"]["fail-fast"], False)

    def test_matrix_covers_three_operating_systems(self):
        matrix = self.job["strategy"]["matrix"]
        self.assertEqual(
            matrix["os"], ["ubuntu-latest", "windows-latest", "macos-latest"]
        )

    def test_matrix_qt_version(self):
        matrix = self.job["strategy"]["matrix"]
        self.assertEqual(matrix["qt-version"], ["6.8.0"])

    def test_env_sets_offscreen_qt_platform(self):
        self.assertEqual(self.job.get("env"), {"QT_QPA_PLATFORM": "offscreen"})

    def test_step_order(self):
        names = [s.get("name") for s in self.job["steps"]]
        self.assertEqual(
            names,
            [
                "Checkout",
                "Install Qt",
                "Install lcov",
                "Patch macOS AGL framework reference in Qt",
                "Configure",
                "Patch generated build files to strip AGL framework on macOS",
                "Build",
                "Test",
                "Collect coverage",
                "Upload coverage to Codecov",
            ],
        )

    def test_install_qt_step_configuration(self):
        step = get_step(self.job, "Install Qt")
        self.assertEqual(step["uses"], "jurplel/install-qt-action@v4")
        self.assertEqual(
            step.get("with"),
            {
                "version": "${{ matrix.qt-version }}",
                "modules": "qtcharts qtshadertools qtquick3d",
                "cache": True,
            },
        )

    def test_lcov_only_installed_on_ubuntu_with_qt6(self):
        step = get_step(self.job, "Install lcov")
        self.assertEqual(
            step.get("if"),
            "matrix.os == 'ubuntu-latest' && startsWith(matrix.qt-version, '6')",
        )
        self.assertIn("lcov", step.get("run", ""))

    def test_macos_agl_qt_patch_is_macos_only(self):
        step = get_step(self.job, "Patch macOS AGL framework reference in Qt")
        self.assertEqual(
            step.get("if"),
            "matrix.os == 'macos-latest' && startsWith(matrix.qt-version, '6')",
        )
        self.assertIn("QT_ROOT_DIR", step.get("run", ""))

    def test_configure_step_sets_coverage_flag_conditionally(self):
        step = get_step(self.job, "Configure")
        run = step.get("run", "")
        self.assertIn("cmake -S . -B build", run)
        self.assertIn("-DCMAKE_BUILD_TYPE=Debug", run)
        self.assertIn(
            "-DQGRAPHPLOT_BUILD_COVERAGE=${{ (matrix.os == 'ubuntu-latest' "
            "&& startsWith(matrix.qt-version, '6')) && 'ON' || 'OFF' }}",
            run,
        )

    def test_configure_step_sets_agl_workaround_on_macos_only(self):
        step = get_step(self.job, "Configure")
        run = step.get("run", "")
        self.assertIn("matrix.os == 'macos-latest'", run)
        self.assertIn("WrapOpenGL_AGL", run)

    def test_build_build_files_patch_is_macos_only(self):
        step = get_step(
            self.job, "Patch generated build files to strip AGL framework on macOS"
        )
        self.assertEqual(
            step.get("if"),
            "matrix.os == 'macos-latest' && startsWith(matrix.qt-version, '6')",
        )

    def test_build_step_command(self):
        step = get_step(self.job, "Build")
        self.assertEqual(step.get("run"), "cmake --build build --parallel --config Debug")

    def test_test_step_uses_xvfb_only_on_ubuntu(self):
        step = get_step(self.job, "Test")
        self.assertEqual(step.get("shell"), "bash")
        run = step.get("run", "")
        self.assertIn('"${{ matrix.os }}" = "ubuntu-latest"', run)
        self.assertIn("xvfb-run ctest --test-dir build --output-on-failure -C Debug", run)
        self.assertIn("ctest --test-dir build --output-on-failure -C Debug", run)

    def test_collect_coverage_is_ubuntu_qt6_only(self):
        step = get_step(self.job, "Collect coverage")
        self.assertEqual(
            step.get("if"),
            "matrix.os == 'ubuntu-latest' && startsWith(matrix.qt-version, '6')",
        )
        run = step.get("run", "")
        self.assertIn("lcov --capture --directory build", run)
        self.assertIn("lcov --remove coverage.info", run)

    def test_upload_coverage_step_configuration(self):
        step = get_step(self.job, "Upload coverage to Codecov")
        self.assertEqual(
            step.get("if"),
            "matrix.os == 'ubuntu-latest' && startsWith(matrix.qt-version, '6')",
        )
        self.assertEqual(step.get("uses"), "codecov/codecov-action@v4")
        with_block = step.get("with", {})
        self.assertEqual(with_block.get("files"), "./coverage.info")
        self.assertEqual(with_block.get("token"), "${{ secrets.CODECOV_TOKEN }}")
        self.assertIs(with_block.get("disable_search"), True)
        self.assertIs(with_block.get("fail_ci_if_error"), True)


if __name__ == "__main__":
    unittest.main()