# KPAD unified absent-remote status — 2026-10-04

The [verified WPADProbe run](HARDWARE_RESULTS_2026-10-04_WPAD_PROBE_KPAD_UNIFIED_FRONTIER.md)
reaches KPADGetUnifiedWpadStatus `0x8019812C`, channel 0, output `0x80398F50`,
count 1, LR `0x8051EEC0`, dispatch 617560 / 107.369 seconds. The user sees
black output followed by an error. This candidate implements that boundary;
the [fresh console result](HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md)
now accepts return through the observed count-1 polling path, with a final
channel-3 report and later PADControlMotor frontier. The user again confirms
black output followed by an error.

## Primary contract and scope

The pinned WiiCompiled `runtime/src/hle/input/kpad.cpp` implementation is
`KPAD__GetUnifiedWpadStatus_HLE` / `WriteUnifiedStatus` at revision
`a135beb201042b20f390c6695ca6b26768820fb4`. The public RMCP01 map attributes
`0x8019812C` to KPADGetUnifiedWpadStatus. No Wii Bluetooth backend is available
on this Horizon path; the configured Switch controller stays on PADRead.

For channels 0–3 with a non-null output and nonzero requested count, write
`min(count,16)` absent samples. Each 56-byte (`0x38`) sample is zeroed,
including extension/IR data and padding, with byte `0x29` = `0xFF`
(WPAD_ERR_NO_CONTROLLER) and byte `0x36` = 2 (core ACC_DPD format). The core
device byte `0x28` is zero. Return valid-sample count **0** in r3. Library
initialization does not change this behavior. Bad channels, null output or
zero count return 0 without accessing output or opening the status report.

Count clamping precedes byte-length calculation. The complete output span
must map before any guest write. Unmapped, partial and wrapping buffers
produce `KPAD_UNIFIED_STATUS_RANGE` and abort with unchanged CPU/memory.
This deliberately narrows the pinned write-time exception recovery, which
can leave partially written entries. All CPU state except the specified
normal return register is preserved; callback, WPAD/PAD library and input
acquisition state are unchanged.

Per-channel transition caching keeps repeated four-channel polls from
reopening the SD report. `fast-track-kpad-unified-status.txt` records channel,
output pointer, requested/written entries, checked byte length, return count
and absent remote backend. Every successful call still writes guest samples.

## Executable validation

[The host contract](../tests/kpad_unified_status_contract.cpp) runs the actual
CPU bridge and Memory slice; only guest allocation, diagnostic capture and
file-open instrumentation are seams. **8,987 cases pass** under ASan, fatal
UBSan and LeakSanitizer with optimization and warnings treated as errors.
Independent literal SDK wire expectations check every byte, all four channels,
initialization states, every single-sample unaligned window, all maximal-count
windows, requests 1–17 and UINT32_MAX, exact ends, full CPU preservation,
library state and repeated SD-report behavior. Invalid buffers verify refusal
before any CPU or guest-memory effect, including partial last entries.

Eight independently compiled mutations are rejected: fabricated return count,
wrong error byte, wrong format byte, missing count cap, partial-buffer
preflight, invalid-channel writes, caller count clobber and repeated SD opens.
The complete synthetic NRO build and both AArch64 configurations pass, with
all three input bridges and their probes retained in the ELF. Script/workflow
lint and 472 local documentation links pass.

Build-switch includes the sixteenth executable suite. AArch64 syntax covers
rendered and synthetic configurations; the synthetic probe and production
bridge are explicitly retained and checked in fast-track CI. All sixteen
local suites pass.

Code revision `aa73a0c51a42496a01b476a9921697ea53dde1da` passes all five
exact-code pull-request workflows / six jobs. The
[actual build-switch run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37230889090)
confirms 8,987 KPAD cases, 556 WPADProbe cases, 65,563 PADRead cases and the
synthetic compile without desktop defines.

The private Rendered Discovery build exits 0. Its NRO is **73,494,584 bytes**,
SHA-256 `fa05c8703c38ab2361e61562b8e8203e9fe99becfd3ea8510706a57435db0645`.
All 48 required strong symbols are retained; 19 scoped symbols have unique
expected providers across 229 host inputs, 19 container Rust archives and
seven named libraries. Checked FIFO/vertex mirrors match their preparation
contract. This provider audit excludes broader symbols and compiler-injected
implicit libraries.

The count-1 polling return is now hardware-crossed. Other input paths, larger
counts, raw output capture, game pixels, replay and sustained gameplay remain
open. Pins, the original nine-file upstream patch and its modification
times, and private-data exclusions are preserved. The private NRO and raw
validation artifacts are excluded from the public repository.

## Launch attempt

At 2026-10-04 20:32:15 UTC, UDP netloader discovery received no reply. The
direct nxlink attempt started at 20:32:48 UTC and exited 1 at 20:32:51 UTC
with a connection failure before transfer. No TCP preflight was used. This
was a transport result before launch, separate from console execution.

The same validated NRO was subsequently sent successfully: direct nxlink
started at **21:41:58 UTC** and exited **0 at 21:42:13 UTC**, transferring
26,743,208 compressed bytes / 2,247 blocks (36.39%). Launch revision
`6a43ec8` differs from the validated code revision only in Markdown. No TCP
preflight was used. USB/MTP retrieval on October 5 supplies 34 verified reports
/ 544,124 bytes and accepts the observed polling return before PADControlMotor
`0x801AF908`, channel 0, command 2, dispatch 618177 / 109.316 seconds.
The user confirms black output followed by an error; game pixels remain
unaccepted. See the linked hardware report for freshness and acceptance limits.
