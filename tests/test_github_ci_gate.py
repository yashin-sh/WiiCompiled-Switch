import copy
import io
import unittest
from unittest.mock import Mock, patch

from scripts import ci_desktop_replay_needed as scope
from scripts import github_ci_gate as gate

REPO = "owner/repository"
SHA = "a" * 40


def check(
    name, conclusion="SUCCESS", status="COMPLETED", run="1", app="github-actions"
):
    return {
        "__typename": "CheckRun",
        "name": name,
        "status": status,
        "conclusion": conclusion,
        "detailsUrl": f"https://github.com/{REPO}/actions/runs/{run}/job/123",
        "checkSuite": {"app": {"slug": app}},
    }


def green():
    return [check(name) for name in sorted(gate.REQUIRED_CHECKS)]


def pr(sha=SHA):
    return {
        "head": {"sha": sha},
        "base": {"ref": "main"},
        "state": "open",
        "draft": False,
    }


def page(nodes, more=False, cursor=None, sha=SHA):
    return {
        "data": {
            "repository": {
                "object": {
                    "oid": sha,
                    "statusCheckRollup": {
                        "contexts": {
                            "nodes": nodes,
                            "pageInfo": {"hasNextPage": more, "endCursor": cursor},
                        }
                    },
                }
            }
        }
    }


class GateContracts(unittest.TestCase):
    def test_all_checks_green(self):
        rows = green() + [check("additional-job")]
        self.assertEqual(gate.evaluate(rows, REPO).status, "success")
        self.assertEqual(gate.evaluate(rows, REPO).checks, 9)

    def test_success_never_hides_cancelled_duplicate(self):
        for rows in [
            green() + [check("nro", "CANCELLED")],
            [check("nro", "CANCELLED")] + green(),
        ]:
            self.assertEqual(gate.evaluate(rows, REPO).status, "failure")

    def test_every_non_success_conclusion_blocks(self):
        for conclusion in [
            "FAILURE",
            "CANCELLED",
            "SKIPPED",
            "NEUTRAL",
            "TIMED_OUT",
            "ACTION_REQUIRED",
            "STARTUP_FAILURE",
            "STALE",
            None,
        ]:
            with self.subTest(conclusion=conclusion):
                self.assertEqual(
                    gate.evaluate(green() + [check("extra", conclusion)], REPO).status,
                    "failure",
                )

    def test_queued_pending_missing_and_empty_are_not_green(self):
        for state in ["QUEUED", "IN_PROGRESS", "WAITING", "REQUESTED"]:
            self.assertEqual(
                gate.evaluate(green() + [check("extra", None, state)], REPO).status,
                "pending",
            )
        self.assertEqual(gate.evaluate([], REPO).status, "pending")
        self.assertEqual(gate.evaluate(green()[1:], REPO).status, "pending")

    def test_additional_legacy_statuses_must_succeed(self):
        for state, expected in [
            ("SUCCESS", "success"),
            ("PENDING", "pending"),
            ("ERROR", "failure"),
            ("FAILURE", "failure"),
        ]:
            row = {"__typename": "StatusContext", "context": "external", "state": state}
            self.assertEqual(gate.evaluate(green() + [row], REPO).status, expected)

    def test_foreign_checks_cannot_replace_required_actions_jobs(self):
        self.assertEqual(
            gate.evaluate(
                [check(n, app="other-app") for n in gate.REQUIRED_CHECKS], REPO
            ).status,
            "pending",
        )
        self.assertEqual(
            gate.evaluate(
                [
                    {"__typename": "StatusContext", "context": n, "state": "SUCCESS"}
                    for n in gate.REQUIRED_CHECKS
                ],
                REPO,
            ).status,
            "pending",
        )

    def test_ci_gate_omits_only_its_own_running_job(self):
        rows = [check(n) for n in gate.BASE_CHECKS]
        own = check(gate.GATE_CHECK, None, "IN_PROGRESS", run="42")
        self.assertEqual(gate.evaluate(rows + [own], REPO, "42").status, "success")
        self.assertEqual(gate.evaluate(rows, REPO, "42").status, "pending")
        self.assertEqual(
            gate.evaluate(
                rows + [own, check(gate.GATE_CHECK, "CANCELLED", run="43")], REPO, "42"
            ).status,
            "failure",
        )
        self.assertEqual(gate.evaluate(rows + [own], REPO).status, "pending")

    def test_unknown_context_type_fails_closed(self):
        self.assertEqual(
            gate.evaluate(green() + [{"__typename": "FutureType"}], REPO).status,
            "failure",
        )

    def test_reads_all_pages_including_failure_after_first_hundred(self):
        first = green() + [check(f"extra-{n}") for n in range(92)]
        api = Mock(
            side_effect=[
                page(first, True, "next"),
                page([check("late-failure", "FAILURE")]),
            ]
        )
        rows = gate.fetch_checks(REPO, SHA, api)
        self.assertEqual(len(rows), 101)
        self.assertIn("cursor=next", api.call_args.args)
        self.assertEqual(gate.evaluate(rows, REPO).status, "failure")

    def test_pagination_errors_and_wrong_commit_fail_closed(self):
        for responses in [
            [page([], True, None)],
            [page([], True, "same"), page([], True, "same")],
            [page([], sha="b" * 40)],
            [{"errors": [{"message": "denied"}]}],
        ]:
            with self.assertRaises(RuntimeError):
                gate.fetch_checks(REPO, SHA, Mock(side_effect=responses))

    def test_merge_rechecks_all_contexts_and_pins_head_without_bypass(self):
        run = Mock()
        gate.merge_checked(
            REPO, 12, SHA, Mock(return_value=green()), Mock(return_value=pr()), run
        )
        args = run.call_args.args[0]
        self.assertEqual(args[-2:], ["--match-head-commit", SHA])
        self.assertNotIn("--admin", args)
        self.assertNotIn("--auto", args)

    def test_merge_head_race_never_runs_merge_command(self):
        run = Mock()
        with self.assertRaises(RuntimeError):
            gate.merge_checked(
                REPO,
                12,
                SHA,
                Mock(return_value=green()),
                Mock(side_effect=[pr(), pr("b" * 40)]),
                run,
            )
        run.assert_not_called()

    def test_new_failure_at_merge_recheck_blocks_merge(self):
        run = Mock()
        with self.assertRaises(RuntimeError):
            gate.merge_checked(
                REPO,
                12,
                SHA,
                Mock(return_value=green() + [check("extra", "FAILURE")]),
                Mock(return_value=pr()),
                run,
            )
        run.assert_not_called()

    def test_draft_closed_or_other_base_blocks_merge(self):
        for key, value in [
            ("draft", True),
            ("state", "closed"),
            ("base", {"ref": "other"}),
        ]:
            data = copy.deepcopy(pr())
            data[key] = value
            run = Mock()
            with self.assertRaises(RuntimeError):
                gate.merge_checked(
                    REPO,
                    12,
                    SHA,
                    Mock(return_value=green()),
                    Mock(return_value=data),
                    run,
                )
            run.assert_not_called()

    def test_gate_mode_cannot_be_used_outside_its_actions_job_or_to_merge(self):
        for arguments in [
            ["--sha", SHA, "--gate-run", "42"],
            ["--pr", "12", "--gate-run", "42", "--merge"],
        ]:
            api = Mock()
            with (
                patch("sys.argv", ["gate", "--repo", REPO, *arguments]),
                patch.dict("os.environ", {}, clear=True),
                patch.object(gate, "gh_json", api),
                patch("sys.stderr", io.StringIO()),
            ):
                with self.assertRaises(SystemExit) as error:
                    gate.main()
                self.assertEqual(error.exception.code, 2)
                api.assert_not_called()

    def test_deleted_app_on_extra_check_is_still_checked(self):
        extra = check("old-app")
        extra["checkSuite"]["app"] = None
        self.assertEqual(gate.evaluate(green() + [extra], REPO).status, "success")
        extra["conclusion"] = "FAILURE"
        self.assertEqual(gate.evaluate(green() + [extra], REPO).status, "failure")

    def test_replay_scope_keeps_original_coverage(self):
        for name in [
            "desktop-gx-replay/capture.cpp",
            "include/gx_linear_texture_descriptor.hpp",
            "source/gx_init_tex_obj_hle_bridge.cpp",
            "tests/gx_texture_load_contract.cpp",
            "scripts/test-gx-texture-load.sh",
            "source/nw4r_lyt_draw_quad_hle_bridge.cpp",
            "tests/lyt_draw_quad_contract.cpp",
            "scripts/test-lyt-draw-quad.sh",
            "scripts/prepare-replay-runtime.py",
            "patches/wiicompiled/m3-wiicompiled-switch-build.patch",
            "source/rendered_fifo_capture.cpp",
            "include/surface_presenter.hpp",
            "third_party/WiiCompiled",
            ".github/workflows/desktop-replay.yml",
            "scripts/ci_desktop_replay_needed.py",
        ]:
            with self.subTest(name=name):
                self.assertTrue(scope.needs_replay([name]))
        self.assertFalse(
            scope.needs_replay(["README.md", "source/gx_load_light_obj_hle_bridge.cpp"])
        )


if __name__ == "__main__":
    unittest.main()
