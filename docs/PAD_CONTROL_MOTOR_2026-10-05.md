# PADControlMotor without an actuator — 2026-10-05

The [verified KPAD run](HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md)
stops at PADControlMotor `0x801AF908`, channel 0, command 2, LR `0x8051EEEC`,
dispatch 618177 / 109.316 seconds. The user observes black output followed
by an error. This candidate implements that exact boundary; its return still
needs fresh console progression evidence.

## Pinned contract and implementation

Pinned WiiCompiled `runtime/src/hle/input/pad.cpp` declares
`PAD__ControlMotor_HLE` as a void native override. Its arguments are signed
channel in r3 and full-width command in r4. It forwards to Aurora
PADControlMotor, first changing RUMBLE to STOP if rumble is disabled.
Aurora's `lib/dolphin/pad/pad.cpp` returns immediately when no actuator device
exists, before interpreting the command. Its input lookup returns null for
unassigned channels. SDK commands are STOP=0, RUMBLE=1 and STOP_HARD=2.

The Horizon PADRead bridge already reports rumble capability mask zero and
has no actuator backend. PADControlMotor therefore preserves the absent-device
void return: no input polling, device construction, guest memory access,
callback or library-state change. Every CPU register, including r3/r4, is
preserved. Invalid and negative channels likewise return without a device.
Unknown command words have no effect in this absent-actuator branch, matching
the pinned early return; this does not implement physical rumble.

The stage is `RMCP01_PAD_CONTROL_MOTOR`. Valid channels record
`no-actuator-pass`, channel, full command, zero capability and absent backend
in `fast-track-pad-control-motor.txt`. Per-channel command caching prevents
repeated polling from reopening the SD report; failed opens remain retryable.
The bridge is independent of rendering and PAD/WPAD initialization.

## Validation

The executable host contract passes **4,152 cases** under ASan, fatal UBSan
and LeakSanitizer. It exercises the production bridge through its CPU
trait with complete byte-for-byte CPU preservation, both initialization states,
all four valid channels, invalid/signed extremes, SDK commands and full-width
unknown values. It checks the observed `(0,2)` report, final channel-3 report,
four-channel polling cache, command transitions and retry after SD-open failure.
Four independently compiled mutations are rejected: void return clobber,
invalid-channel indexing, repeated SD opens and caching a failed SD open.
It links without guest Memory or an input backend, so accidental dependencies
on either fail the executable link.

The new seventeenth host suite and synthetic probe/production symbol retention
are wired into CI. The full synthetic NRO build passes, retaining all four
input bridges/probes. All seventeen local suites, script/workflow lint and
both AArch64 configurations pass. Dependency pins, the original upstream
patch bytes and its modification times are preserved.

Code `1cff562ad888f917a2ffd9c586e5e3cab47cb765` passes all five exact-code
pull-request workflows / six jobs. The
[actual build-switch log](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37265909786)
confirms 4,152 PADControlMotor cases, 8,987 KPAD cases, 556 WPADProbe cases,
65,563 PADRead cases and the new synthetic compile gate without desktop defines.

The private Rendered Discovery build exits 0. Its NRO is **73,494,584 bytes**,
SHA-256 `b4cb13ebb58a6cf7a1a567e5ed89ce73df0f7bdf905cc266fa7b6856d8a1feda`.
All 49 required strong symbols are retained. Twenty scoped native, bridge,
flag and FIFO symbols have unique expected providers across 230 host inputs,
nineteen container Rust archives and seven named libraries. Checked FIFO and
vertex mirrors match their preparation contract. This audit does not cover
broader symbols or compiler-injected implicit libraries.

## Console deployment

UDP netloader discovery on **2026-10-05 05:26:31 UTC** receives no reply.
No TCP preflight is used. USB/MTP copies the validated NRO from **05:27:33
to 05:27:43 UTC** to
`sdmc:/switch/WiiCompiled-Switch-pad-control-motor-rendered-discovery.nro`.
A complete readback matches all 73,494,584 bytes and the SHA-256 above.
This USB deployment does not launch the application or accept PADControlMotor's
hardware return.

On **2026-10-05 17:13:07 UTC**, the same validated NRO subsequently transferred
via direct nxlink with **exit 0**, sending 26,743,203 compressed bytes in
2,247 blocks (36.39%). The source revision is `1cff562`; launch revision
`0d230ed` differs only in Markdown and is included in merged PR #313.
The launch checks reverified all non-Markdown candidate hashes, the NRO size
and SHA-256, dependency pins and the original upstream patch before transfer.
UDP discovery at 17:12:23 UTC had no reply; the actual transfer nevertheless
succeeded without a TCP preflight. Transfer success does not establish the
native return or visible game output. Fresh reports and the user's observation
from this launch remain pending; they must establish progression beyond
`0x801AF908`.

The preceding KPAD run remains the hardware baseline. PADControlMotor return,
physical vibration, recognizable game pixels, sustained gameplay and later
unknown calls remain outside hardware acceptance. Private products and raw
diagnostics remain excluded from Git.
