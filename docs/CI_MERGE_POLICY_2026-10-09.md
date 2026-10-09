# Every check must be green before merging — 2026-10-09

The previous merge helper selected a successful run for each of five workflows.
It ignored other checks on the same PR HEAD. This allowed merges with cancelled
checks still visible and did not satisfy the user's requirement that every check
be green.

The audit found these initial and repaired rollups. Each interrupted workflow
was rerun on its original commit; no check or validation suite was removed.

| PR | Initial successful checks | Initial cancelled checks | After reruns |
| --- | --- | --- | --- |
| [#339](https://github.com/yashin-sh/WiiCompiled-Switch/pull/339) | 8 | 5 | 13/13 successful |
| [#341](https://github.com/yashin-sh/WiiCompiled-Switch/pull/341) | 6 | 6 | 12/12 successful |
| [#344](https://github.com/yashin-sh/WiiCompiled-Switch/pull/344) | 7 | 5 | 12/12 successful |
| [#345](https://github.com/yashin-sh/WiiCompiled-Switch/pull/345) | 6 | 6 | 12/12 successful |

Eighteen workflow runs were rerun. Their original cancellations remain part of
Actions history. Repairing their current rollups does not retroactively justify
the earlier merge decisions.

## Trigger and merge correction

Development branches now run validation through `pull_request`; `push` runs it
on `main`. This removes the competing push/PR runs that shared a concurrency
group and cancelled one another on the same commit. Superseded PR commits may
still be cancelled when a newer commit arrives; the new HEAD must pass fully.

The replay check is always present. Its existing path scope now runs inside the
job: changed replay inputs execute the full existing desktop suite, and unchanged
inputs execute the scope check and report that the replay suite was unnecessary.
The validation commands remain intact, and the check itself is never skipped.

`all-checks-green` reads every page of the exact commit's GitHub status rollup.
Every published check must finish successfully, including duplicate contexts and
additional checks outside the required set. Failure, cancellation, timeout,
neutral/skipped results, pending/missing checks and API errors prevent merging.
The aggregate job omits only its own currently running check to avoid waiting on
itself. The merge command includes that check and re-reads the HEAD and all
contexts immediately before requesting the merge.

The eight required Actions checks are `lint`, `gx-contracts`, `nro`,
`synthetic-fast-track`, `synthetic-bootstrap-prelude`, `synthetic-sequence`,
`synthetic-replay` and `all-checks-green`. Server-side protection on `main` requires
all eight, requires an up-to-date branch and applies to administrators. The CLI
uses `--match-head-commit` and does not bypass protection. Renderer changes still
need their separate private rendered build before this command is used:

```text
python3 scripts/github_ci_gate.py --pr <number> --wait --merge --proof /tmp/ci-proof.json
```

The public contracts cover a successful run mixed with a cancelled duplicate,
non-success conclusions, external statuses, missing/queued checks, failures after
the first hundred contexts, API/pagination errors and HEAD changes during the
merge check. Only the current gate job can omit itself; that CI mode cannot merge.
GitHub CI and the merge command must pass before this correction is merged.
