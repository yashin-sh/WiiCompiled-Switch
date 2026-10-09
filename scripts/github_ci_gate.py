#!/usr/bin/env python3
"""Fail closed unless every published check on an exact commit succeeds."""

import argparse
import json
import os
import re
import subprocess
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

BASE_CHECKS = frozenset(
    {
        "lint",
        "gx-contracts",
        "nro",
        "synthetic-fast-track",
        "synthetic-bootstrap-prelude",
        "synthetic-sequence",
        "synthetic-replay",
    }
)
GATE_CHECK = "all-checks-green"
REQUIRED_CHECKS = BASE_CHECKS | {GATE_CHECK}
QUERY = """
query($owner:String!, $name:String!, $oid:String!, $cursor:String) {
  repository(owner:$owner, name:$name) {
    object(expression:$oid) {
      ... on Commit {
        oid
        statusCheckRollup {
          contexts(first:100, after:$cursor) {
            nodes {
              __typename
              ... on CheckRun { name status conclusion detailsUrl checkSuite { app { slug } } }
              ... on StatusContext { context state targetUrl }
            }
            pageInfo { hasNextPage endCursor }
          }
        }
      }
    }
  }
}
"""


@dataclass(frozen=True)
class Result:
    status: str
    blockers: tuple[str, ...]
    checks: int


def gh_json(*args):
    return json.loads(subprocess.check_output(["gh", *args], text=True))


def fetch_checks(repo, sha, api=gh_json):
    owner, name = repo.split("/", 1)
    cursor = None
    seen = set()
    checks = []
    while True:
        args = [
            "api",
            "graphql",
            "-f",
            f"query={QUERY}",
            "-f",
            f"owner={owner}",
            "-f",
            f"name={name}",
            "-f",
            f"oid={sha}",
        ]
        if cursor is not None:
            args += ["-f", f"cursor={cursor}"]
        response = api(*args)
        if response.get("errors"):
            raise RuntimeError(f"GitHub query failed: {response['errors']}")
        commit = response["data"]["repository"]["object"]
        if not commit or commit.get("oid") != sha:
            raise RuntimeError("GitHub did not return the exact requested commit")
        rollup = commit["statusCheckRollup"]
        if not rollup:
            return checks
        page = rollup["contexts"]
        checks.extend(page["nodes"])
        if not page["pageInfo"]["hasNextPage"]:
            return checks
        cursor = page["pageInfo"]["endCursor"]
        if not cursor or cursor in seen:
            raise RuntimeError("Invalid check pagination; refusing partial evidence")
        seen.add(cursor)


def evaluate(checks, repo, gate_run=None):
    names = set()
    failed = []
    pending = []
    counted = 0
    own = 0
    own_url = f"https://github.com/{repo}/actions/runs/{gate_run}/job/"
    for check in checks:
        kind = check.get("__typename")
        if kind == "CheckRun":
            name = check["name"]
            app = ((check.get("checkSuite") or {}).get("app") or {}).get("slug")
            if (
                gate_run is not None
                and app == "github-actions"
                and name == GATE_CHECK
                and (check.get("detailsUrl") or "").startswith(own_url)
            ):
                own += 1
                continue
            counted += 1
            if app == "github-actions":
                names.add(name)
            if check["status"] != "COMPLETED":
                pending.append(f"{name}: {check['status']}")
            elif check.get("conclusion") != "SUCCESS":
                failed.append(f"{name}: {check.get('conclusion')}")
        elif kind == "StatusContext":
            counted += 1
            name = check["context"]
            if check["state"] == "PENDING":
                pending.append(f"{name}: PENDING")
            elif check["state"] != "SUCCESS":
                failed.append(f"{name}: {check['state']}")
        else:
            failed.append(f"Unknown check type: {kind}")
    required = BASE_CHECKS if gate_run is not None else REQUIRED_CHECKS
    pending.extend(f"Missing check: {name}" for name in sorted(required - names))
    if gate_run is not None and own != 1:
        pending.append(f"Expected exactly one current gate check, found {own}")
    if failed:
        return Result("failure", tuple(failed + pending), counted)
    if pending:
        return Result("pending", tuple(pending), counted)
    return Result("success", (), counted)


def fetch_pr(repo, number, api=gh_json):
    return api("api", f"repos/{repo}/pulls/{number}")


def require_head(pr, sha, merging=False):
    if pr["head"]["sha"] != sha:
        raise RuntimeError("PR HEAD changed; validate the new commit before merging")
    if merging and (pr["state"] != "open" or pr["draft"]):
        raise RuntimeError("Only an open, non-draft PR may be merged")
    if merging and pr["base"]["ref"] != "main":
        raise RuntimeError("This merge gate only authorizes the main branch")


def merge_checked(
    repo, number, sha, checks_api=fetch_checks, pr_api=fetch_pr, run=None
):
    """Re-read every check and pin the merge to the same reviewed HEAD."""
    require_head(pr_api(repo, number), sha, merging=True)
    result = evaluate(checks_api(repo, sha), repo)
    if result.status != "success":
        raise RuntimeError(f"Merge blocked: {result.blockers}")
    require_head(pr_api(repo, number), sha, merging=True)
    args = [
        "gh",
        "pr",
        "merge",
        str(number),
        "--repo",
        repo,
        "--merge",
        "--match-head-commit",
        sha,
    ]
    (run or subprocess.run)(args, check=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo")
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--pr", type=int)
    target.add_argument("--sha")
    parser.add_argument("--wait", action="store_true")
    parser.add_argument("--timeout", type=int, default=3300)
    parser.add_argument("--merge", action="store_true")
    parser.add_argument("--proof", type=Path)
    parser.add_argument(
        "--gate-run", help="CI-only: ignore this running gate, never merge"
    )
    args = parser.parse_args()
    repo = (
        args.repo or gh_json("repo", "view", "--json", "nameWithOwner")["nameWithOwner"]
    )
    if not re.fullmatch(r"[\w.-]+/[\w.-]+", repo) or args.timeout <= 0:
        parser.error("Invalid repository or timeout")
    if args.merge and (args.pr is None or args.gate_run is not None):
        parser.error("Merge requires a PR and includes the gate itself")
    if args.gate_run is not None and (
        args.sha is None
        or os.environ.get("GITHUB_ACTIONS") != "true"
        or os.environ.get("GITHUB_RUN_ID") != args.gate_run
        or os.environ.get("GITHUB_JOB") != GATE_CHECK
    ):
        parser.error("Only this running Actions gate can omit its own pending check")
    sha = args.sha or fetch_pr(repo, args.pr)["head"]["sha"]
    if not re.fullmatch(r"[a-f0-9]{40}", sha):
        parser.error("Expected a complete commit SHA")
    deadline = time.monotonic() + args.timeout
    previous = None
    while True:
        if args.pr is not None:
            require_head(fetch_pr(repo, args.pr), sha, merging=args.merge)
        checks = fetch_checks(repo, sha)
        result = evaluate(checks, repo, args.gate_run)
        row = {
            "status": result.status,
            "head": sha,
            "checks": result.checks,
            "blockers": result.blockers,
            "merge_eligible": result.status == "success" and args.gate_run is None,
        }
        if row != previous:
            print(json.dumps(row), flush=True)
            previous = row
        if result.status == "success":
            if args.proof:
                args.proof.write_text(
                    json.dumps(
                        {
                            **row,
                            "checked_utc": datetime.now(timezone.utc).isoformat(),
                            "contexts": checks,
                        },
                        indent=2,
                    )
                    + "\n"
                )
            if args.merge:
                merge_checked(repo, args.pr, sha)
                merged = fetch_pr(repo, args.pr)
                if not merged["merged"] or merged["head"]["sha"] != sha:
                    raise RuntimeError(
                        "Merge readback did not confirm the validated HEAD"
                    )
                print(f"Merged PR #{args.pr} after every check succeeded", flush=True)
            return 0
        if result.status == "failure" or not args.wait or time.monotonic() >= deadline:
            return 1
        time.sleep(15)


if __name__ == "__main__":
    raise SystemExit(main())
