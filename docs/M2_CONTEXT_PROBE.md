# M2 — AArch64 HostContext probe

This probe validates the cooperative context-switch mechanism required by WiiCompiled's guest scheduler on Horizon/libnx. It contains no Nintendo game data.

## What it tests

1. Creates a dedicated 256 KiB worker stack.
2. Initializes an AArch64 context compatible with the Switch port's `HostContext` design.
3. Performs scheduler -> worker -> scheduler handoffs.
4. Verifies continuation after resuming the same context.
5. Seeds and verifies AAPCS64 callee-saved integer registers `x19-x29` and SIMD registers `d8-d15` across an actual switch-away/switch-back round-trip.
6. Performs 100,000 cooperative scheduler/worker round-trips and records elapsed system ticks.

The implementation conservatively saves `x18` and the full `q8-q15` values even though the ABI test only requires the callee-saved portions.

## Run on hardware

Use the same `.nro` as the VM probe. It now executes both probes automatically.

Copy it to:

`sdmc:/switch/WiiCompiled-Switch/WiiCompiled-Switch.nro`

Launch it through hbmenu under Atmosphère. When the screen says that all probes are complete, press `+` to exit.

Both VM and HostContext results are stored in:

`sdmc:/switch/WiiCompiled-Switch/vm-probe.txt`

Send that file back for analysis.

## Pass criteria

A strong pass is:

- worker stack: `OK`
- context init: `OK`
- first handoff: `OK`
- continuation: `OK`
- callee-saved registers: `OK`
- stress switches: `100000 / 100000`
- stress result: `OK`

If any context test fails, do not proceed to integrate this assembly into the full WiiCompiled runtime until the ABI issue is understood.
