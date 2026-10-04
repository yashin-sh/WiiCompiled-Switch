# WPADProbe absent-remote bridge — 2026-10-04

The [latest verified console run](HARDWARE_RESULTS_2026-10-04_PAD_READ_WPAD_PROBE_FRONTIER.md)
accepts PADRead returning and reaches PAL WPADProbe `0x801C0990`, channel 0,
type pointer `0x80398F40`, LR `0x8051EEC0`, dispatch 617055. This candidate
implements that observed boundary. Its hardware return is not yet accepted.

## Contract and scope

The pinned primary implementation is `runtime/src/hle/input/wpad.cpp`,
`WPADProbe_HLE`, at WiiCompiled revision
`a135beb201042b20f390c6695ca6b26768820fb4`; errors and extension values come
from `runtime/include/hle/controller_status_contract.h`. The local RMCP01
symbol map attributes `0x801C0990` to WPADProbe.

Horizon currently has no Wii Bluetooth remote backend. The existing Switch
controller belongs to PADRead, so this bridge follows the pinned absent-remote
branch for channels 0–3: optionally write the four-byte big-endian core type
`0`, and return `WPAD_ERR_NO_CONTROLLER` (`-1`) in r3. Probe does not depend
on WPADInit. Channels outside 0–3 return `WPAD_ERR_BAD_CHANNEL` (`-6`) before
accessing the output pointer. A null pointer is allowed.

Every non-null valid-channel output must have its entire four-byte range
mapped. Unmapped, partial and wrapping ranges produce the durable
`WPAD_PROBE_TYPE_RANGE` diagnostic and abort before modifying the CPU or
guest memory. All CPU state other than the returned r3 is preserved. No
controller polling, callback invocation, initialization change or SDL device
construction occurs.

Per-channel transition caching prevents repeated four-channel polling from
reopening the SD report; guest output is still written on every call. Tests
exercise repeated polling before and after initialization.

The transition report `fast-track-wpad-probe.txt` records the channel,
pointer, result and expected core type, with `remote_backend=absent`. The
type field describes the contract value; an invalid range or null pointer
does not receive a write.

## Validation

[The executable contract](../tests/wpad_probe_contract.cpp) runs the actual
CPU bridge and Memory slice, with only guest allocation and diagnostic seams.
**556 cases pass** with ASan, fatal UBSan and LeakSanitizer, optimization and
warnings treated as errors. Independent wire expectations cover all four
channels before/after initialization, every aligned or unaligned four-byte
window of the test region, exact-end writes, both-side canaries, null output,
invalid-channel precedence, signed errors, unchanged library state and full
CPU preservation. Forked refusal cases verify diagnostics and unchanged
CPU/memory before aborting.

Six independently compiled mutations are rejected: fabricated connection,
wrong extension type, partial-range validation, invalid-channel writes and
clobbered CPU registers and repeated SD report opens.

Build-switch includes this fifteenth host contract. The actual AArch64 syntax
gate compiles WPADProbe in rendered and synthetic configurations. The
synthetic probe is explicitly retained by the linker; fast-track checks the
probe and production bridge symbols in the ELF.

All fifteen local contracts, the full synthetic build, both AArch64
configurations and script/workflow lint pass. Only WPADProbe source/test/script
changed for report deduplication; the other fourteen suites have unchanged
runtime, header and test inputs. Six mutation checks are rejected.

The initial code `96ea759` passed all five GitHub workflows / six jobs, including
the [actual build-switch run](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37212211362). Its private build was deliberately
stopped (exit 137) to avoid repeated SD report writes; no NRO from that attempt
was launched. Completed objects are reused. Final-code GitHub Actions and the
private Rendered Discovery NRO are being validated. Console return, per-button PAD behavior and
recognizable game pixels remain open. Dependency pins, the existing upstream
patch and private-data exclusions are preserved.
