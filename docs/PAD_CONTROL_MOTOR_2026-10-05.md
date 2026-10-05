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
input bridges/probes. The AArch64 syntax gate compiles this bridge in rendered
and synthetic configurations. Remaining local suites, exact-code workflows,
private NRO build and hardware launch results will be recorded after completion.

The preceding KPAD run remains the hardware baseline. PADControlMotor return,
physical vibration, recognizable game pixels, sustained gameplay and later
unknown calls remain outside hardware acceptance. Private products and raw
diagnostics remain excluded from Git.
