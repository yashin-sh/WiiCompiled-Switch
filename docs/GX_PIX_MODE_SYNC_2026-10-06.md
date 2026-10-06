# GXPixModeSync candidate — 2026-10-06

The [fresh GXCopyTex run](HARDWARE_RESULTS_2026-10-06_GX_COPY_TEX_PIX_MODE_SYNC_FRONTIER.md)
returns from native copy and stops at GXPixModeSync `0x8016EB70`, stage
GX_COPY_TEX, 120.375 seconds. The checked Mii caller invokes this no-argument
boundary immediately after the copy. Black then error remains the observed
visual result; copied pixels and recognizable game images are unproven.

## Contract

Follow pinned `runtime/src/hle/gx/gx_stubs.cpp`: read the guest GXData pointer,
if nonzero attempt a zero halfword at GXData + 2, swallow guest-memory exceptions,
then call real Aurora GXPixModeSync. Preserve the original unsigned address
wraparound and best-effort semantics. A missing pointer, unreadable pointer or
incomplete halfword does not suppress the native command. Preserve all CPU
bytes and all guest memory except the admitted two-byte update.

Aurora `GXManage.cpp` writes its current `peCtrl` word with the BP opcode 0x61,
then sets `bpSent` to 1. This bridge calls that existing implementation rather
than emitting a guessed guest register or omitting synchronization. No new
frame lifecycle, drain or forced CPU download is added by this boundary.
Native exceptions produce a durable `GX_PIX_MODE_SYNC_NATIVE_EXCEPTION` refusal.
Headless calls durably refuse before guest memory or native work; null CPU
contexts have no effects. The synthetic retained probe is not run at startup.

## Validation and evidence limits

The production trait/bridge is tested with the real Switch Memory slice and
guarded allocation seams. Exhaustive initial halfword values, partial/unmapped
pointer/data, unaligned data, unsigned wraparound, null pointers, reset memory,
CPU/canary preservation and guest-write-before-native order are checked. Native
exception and headless refusals require report/abort proof over a pipe plus
real SIGABRT; unrelated child failure cannot satisfy the proof.

A separate fixture executes the pinned native function and its BP command
macro verbatim. Storage and FIFO transport are seams. It checks both command
writes, their widths/values, preserved unrelated bytes and the flag update
occurring after transport. It does not establish physical GPU completion.

The targeted contract passes **65,559 rendered returns**, one diagnosed native
exception and two diagnosed headless refusals. The pinned BP fixture passes
**65,536 cases**, with ASan, fatal UBSan and LeakSanitizer active. All **21 local suites** pass at
`bed9d2965a0c3add48d702c1bdeed713166f2802`. Six separately compiled defects
are rejected: missing mirror, missing native command, wrong write width,
native-before-mirror, CPU clobber and headless success. Every mutant compiles
successfully and fails its contract; compiler failure is not a passing mutant.

The full rendered devkitA64 gate and full synthetic NRO/ELF build pass, with
the new probe and bridge retained. All five actual exact-code PR workflows /
six jobs pass. Actual [build-switch evidence](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37513014982)
confirms the new contract counts and rendered compilation; actual
[fast-track evidence](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37513014934)
confirms both retained symbols. Script/workflow lint and new C++ formatting
pass. Final documentation-head checks are separate from exact-code evidence.

The private Rendered Discovery build runs in the immutable pinned environment,
with network disabled and six jobs, from **18:42:32 to 19:09:34 UTC**.
All tracked candidate files remain byte-identical during compilation. The
final ELF retains **65 required strong functions** plus canonical
`g_texCopyState`. **39 scoped symbols** have one expected provider each across
**234 host inputs, 19 Rust archives and seven named libraries**. The new native
command comes from GXManage, the bridge from its own translation unit. Checked
FIFO and vertex mirrors match the preparation contract. Broader duplicate
symbols and compiler-injected implicit libraries remain outside this audit.
Dependency pins and the original nine-file upstream patch bytes/nanosecond
modification times remain unchanged.

## Console candidate

The exact NRO is **73,556,024 bytes**, SHA-256
`fefaaf0e40a9b6553f18746ae64d8b1dfdb6708f1053c7537b981a16f0c630df`:
`WiiCompiled-Switch-gx-pix-mode-sync-rendered-discovery.nro`.
Direct nxlink starts at **19:14:05 UTC** and exits **0 at 19:14:18 UTC**,
sending **26,774,763 compressed bytes / 2,249 blocks (36.40%)**. Launch revision
`b38a69aff9130a85b213556e8a65f2b26a06a2bf` differs from validated code only
in Markdown; candidate hashes, pins and upstream patch are rechecked before
transfer. No SD deployment is claimed. Fresh reports, this launch's visual
observation and native console return remain pending; the preceding GXCopyTex
run remains the latest verified console return evidence. Private game products, NROs and raw diagnostic archives stay
excluded. GPU completion and recognizable game pixels remain open.
