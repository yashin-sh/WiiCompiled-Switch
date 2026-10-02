# Port, scripts and CI audit — 2026-10-03

## Assessment and scope

The architecture and evidence-driven bring-up method are coherent, but this
is an incomplete experimental port. Neither passing CI nor this audit proves
bug-free code, full Wii compatibility, recognizable game pixels or playable
performance. The last accepted hardware frontier remains ScaleManually
`0x80171180`; reports for the coordinate NRO `91a4a01` / `64ba8377…` are pending.

The audit reviewed all five workflow definitions, the 31 existing shell/Python
scripts and the current documentation. Manual runtime review focused on memory,
dispatch/ABI, cooperative context switching and rendered frame lifecycle;
automated gates cover the wider compiled port. This is not a line-by-line
proof of every translated game function or every native subsystem.

## Corrected findings

| Area | Defect and resulting behavior |
| --- | --- |
| Memory | `start + length` could wrap in uint64_t and return a pointer inside a tiny allocation for SIZE_MAX bytes. Bounds now use subtraction before addition; the new executable contract failed before the fix. |
| GuestFlat initialization | The analogous guest/backing range additions now reject overflowing sizes before allocation and page rounding. |
| Host context | Stack alignment could wrap for near-SIZE_MAX sizes. The allocation boundary is reviewed and tested separately from the AArch64 context switch. |
| Presentation | After Aurora completed a frame, a false Present result skipped cleanup and retained an active-frame flag for consumed buffers. Cleanup now precedes the success/failure branch. |
| Renderer diagnostics | Teardown exceptions could be followed by RESULT=PASS. The final result now reflects that failure. |
| Rendered syntax gate | Restoring pinned files before/after compilation overwrote pre-existing edits. The gate reuses an applied patch, refuses conflicting edits and only undoes its own temporary patch. |
| Host tests | Four GX scripts allowed UBSan recovery and could print PASS after a runtime error. All executable contracts now make sanitizer errors fatal. |
| Shell command output | DTK follow-up commands used JSON quoting, which did not protect shell substitutions in paths. They now use shell quoting with a regression case. |
| Forecasting | UNPROVEN_ARGS bridges were not consistently classified as constrained native mappings. The scanner now recognizes the UNPROVEN family and has a self-test. |
| Diagnostic bundles | Repeated reads could produce inconsistent report/hash/raw bytes, and interrupted writes could replace a valid bundle. Inputs are captured once; ZIP metadata is deterministic and output replacement atomic; input/output collisions are rejected. |
| PPC scanning | The texture scanner now checks ELF ISA and guest-address range before interpreting instructions; invalid CLI addresses fail cleanly. |
| Build preparation | Already present pins/patches and identical probe source are reused; rendered preparation carries its intended mode instead of toggling OFF then ON and building an unrelated triangle probe. Cleanup and ambiguous output selection have explicit failure boundaries. |
| CI | Main pushes were excluded; permissions, timeouts and concurrency were incomplete; action tags floated; PR checkout differed from the exact-HEAD policy. All five workflows now check the candidate HEAD, include main, use read-only tokens and immutable action revisions, and bound/cancel overlapping work. Actionlint's archive is checksum-verified before extraction. |
| Formatting gate | Failure to resolve a diff base could become an empty-file PASS. The base is checked and the merge-base used before selecting candidate changes. |
| Documentation | September snapshots/frontiers were presented as current. README, roadmap, index and current guides now identify the last accepted run, pending coordinate run, audited batching and evidence limits. |

The runtime fixes do not expand guest dispatch coverage or claim a new
hardware crossing. The presentation-failure path has been attributed to the
pinned Aurora lifecycle, not reproduced as a failure on the console. The host
context allocation test does not execute the handwritten AArch64 switch.

## GitHub configuration and reproducibility

The last five remote workflow runs inspected were successful on `e8f870b…`,
before these local audit changes. The [recorded build run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/36986236474)
is historical evidence, not validation of the new candidate.

The active rules for main require a PR and protect against deletion/force push.
They do **not** contain required status checks. Before relying on CI as a merge
gate, require the six actual job contexts: `lint`, `synthetic-fast-track`,
`synthetic-bootstrap-prelude`, `synthetic-sequence`, `gx-contracts` and `nro`.
This audit reads that configuration; it does not change repository rules.

Actions retain their v4 major versions and are pinned to official commit SHAs;
credentials are not persisted. This follows GitHub's
[security guidance](https://docs.github.com/en/actions/reference/security/secure-use).
Concurrency saves overlapping push/PR runs on the same source branch, with
fork/tag separation; sequential duplicate events can still run. See
[GitHub concurrency semantics](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency).

`devkitpro/devkita64:latest`, Ubuntu and apt-provided compilers still float.
The public registry resolved the devkitPro image to
`sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`
during this audit, but that exact container has not been validated here.
Do not claim a reproducible toolchain until a selected digest and host-tool
versions have passed the required gates. Actionlint's checksum is pinned from
the [official release manifest](https://github.com/rhysd/actionlint/releases/download/v1.7.12/actionlint_1.7.12_checksums.txt).

## Validation and outstanding work

Validation of the final audit candidate is in progress. Results and candidate
identity will be recorded after the checks complete. Public synthetic NRO
builds and nm assertions prove compilation/link retention; they do not run
the NROs on Switch. Executable host contracts cover their stated argument and
memory domains, not every resource/scheduler/audio/input behavior.

The separate private rendered build remains required. Its broad
`--allow-multiple-definition` linker policy deserves an explicit symbol
ownership audit; this review does not prove that every selected definition is
the intended one. Exact-state HLE guards and large texture-descriptor tables
are useful bring-up boundaries but need contract-based generalization before
broader compatibility can be claimed.

Renderer error handling also needs broader fault-injection coverage: a partial
GXCopyDisp exception still returns without a phase-specific recovery contract,
and an outer frame-begin retry deadline cannot interrupt the pinned Aurora
staging-map wait. No retrieved hardware report establishes either failure.
The display-list writer also inherits the pin's write-before-wrap ordering:
a write after an exact fill can cross the list's logical capacity while still
being inside mapped guest RAM. That separate stateful recording contract needs
hardening and targeted tests before its overflow path is accepted.

The GCC return normalizer is scoped to the pinned emitter's output forms.
Its regex transformation is not a general C++ parser: arbitrary string/lambda
forms can be rewritten incorrectly. No corruption of the current generated
shards was established. The headless incremental helper also keys its cache
on HEAD, so documentation-only commits can trigger a full rebuild; content-based
cache identities remain an optimization task.

Continue with the pending console reports before accepting the coordinate
batch. Then use the actual next frontier to select a bounded candidate.
Input mapping, audio output, remaining Wii services, pixel correctness and
representative-scene profiling are still open. See the updated
[`ARCHITECTURE.md`](ARCHITECTURE.md) and
[`FAST_TRACK_VALIDATION_POLICY.md`](FAST_TRACK_VALIDATION_POLICY.md).
