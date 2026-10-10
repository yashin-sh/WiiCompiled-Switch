# Repository instructions

Keep GitHub up to date with completed code, scripts, CI, documentation and
hardware-result work, as requested by the user. Commit and push finished
changes to the development branch and keep its pull request aligned with the
current implementation and validation evidence.

Publish public repository files only. Preserve the existing exclusions for
local game data, generated products, private NROs and raw diagnostic archives.

On every Switch deployment, clean up older NROs deployed by this project after
the new candidate passes complete SD size/SHA-256 readback. Back up each old
candidate locally, verify it against its recorded size/SHA-256, then remove it
and verify the SD directory contains only the current project candidate.
Preserve other applications and game data. Keep cleanup receipts private.

Before merging into main, every published check in the exact PR HEAD rollup must
be completed and successful. Cancelled, skipped, neutral, failed, pending or
missing checks block the merge. Check the complete rollup, with pagination;
never substitute one successful workflow run for the other visible checks.
Use `python3 scripts/github_ci_gate.py --pr <number> --wait --merge` and preserve
the separate private rendered-build gate for renderer changes. Never bypass
branch protection or remove failing checks to obtain a green result. Fix failures
and rerun interrupted checks before merging.
