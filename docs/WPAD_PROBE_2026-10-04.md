# WPADProbe absent-remote bridge — 2026-10-04

The [latest verified console run](HARDWARE_RESULTS_2026-10-04_PAD_READ_WPAD_PROBE_FRONTIER.md)
accepts PADRead returning and reaches PAL WPADProbe `0x801C0990`, channel 0,
type pointer `0x80398F40`, LR `0x8051EEC0`, dispatch 617055. This candidate
implements that observed boundary. Its channel 0 hardware return is now accepted.

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
wrong extension type, partial-range validation, invalid-channel writes,
clobbered CPU registers and repeated SD report opens.

Build-switch includes this fifteenth host contract. The actual AArch64 syntax
gate compiles WPADProbe in rendered and synthetic configurations. The
synthetic probe is explicitly retained by the linker; fast-track checks the
probe and production bridge symbols in the ELF.

All fifteen local contracts, the full synthetic build, both AArch64
configurations and script/workflow lint pass. Only WPADProbe source/test/script
changed for report deduplication; the other fourteen suites have unchanged
runtime, header and test inputs. Six mutation checks are rejected.

Final code `2941f1de49e9d5442251347c53cc5e3bc921cf72` passes all five
GitHub workflows / six jobs. The [actual build-switch log](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37213631951)
confirms 556 WPADProbe cases, 65,563 PADRead cases and synthetic-mode compilation.
The private Rendered Discovery NRO build passes with **47 retained strong
symbols** and **18 scoped unique providers**, checked across 228 host inputs,
19 Rust archives and seven named image libraries. Checked FIFO/vertex mirrors,
source hashes, dependency pins and all original upstream patch bytes and nine
nanosecond mtimes are verified. The immutable local image ran with network
disabled and six compiler tasks. Broader provider ownership remains unaudited.

The NRO is **73,490,488 bytes**, SHA-256
`82685e875a2a7dfcdb76ebe627e2dd82da8bac46fe79a0643f572e9fd660f712`.
The initial `96ea759` candidate also passed its five workflows, but its private
build was deliberately stopped (exit 137) for per-channel report deduplication.
No artifact from that attempt was launched; completed objects were reused.

The earlier final-candidate nxlink attempt at **2026-10-04 15:53:06 UTC** exited 1
with a connection failure before transfer. The known IP responds to ICMP,
USB/MTP is detected and UDP discovery has no netloader reply. This is not a
new game crash or evidence of WPADProbe return. That attempt occurred before the user returned to the netloader; a later
transfer succeeded as recorded below.

The [fresh hardware result](HARDWARE_RESULTS_2026-10-04_WPAD_PROBE_KPAD_UNIFIED_FRONTIER.md)
records transfer exit 0 at 19:44:01 UTC, 33 verified reports / 542,797 bytes
and channel 0 return before KPADGetUnifiedWpadStatus `0x8019812C`. The user
confirms black output followed by an error. Other WPAD channels, raw type-word
capture, per-button PAD behavior and recognizable game pixels remain open. Private game
data, NROs and raw diagnostic archives remain excluded from publication.
