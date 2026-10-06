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
**65,536 cases**, with ASan, fatal UBSan and LeakSanitizer active. Broader local
suites, mutation checks, exact-head workflows, both AArch64 modes, the private
rendered build and console return are pending. The previous GXCopyTex NRO
remains the only candidate with fresh console return evidence.
