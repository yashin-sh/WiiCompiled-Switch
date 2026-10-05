# PADRead returned; WPADProbe frontier — 2026-10-04

The fresh run accepts PADRead returning on the Switch and reaches the already
translated PADClampCircle2. The new DIRECT boundary is WPADProbe
`0x801C0990`, at dispatch **617055 / 108.377 seconds**. Current visual
observation is pending; recognizable Mario Kart Wii pixels remain unproven.

## Candidate and verified retrieval

The Rendered Discovery NRO contains code
`7ca14a76fea64d3b7ff945e34fce8a7ff9efa7c9`; launch HEAD `d732251` only adds
validation documentation. Fourteen local suites, five rejected PADRead mutants,
the complete synthetic build, five exact-code workflows / six jobs and the
private rendered build pass. The NRO is 73,478,200 bytes, SHA-256
`d8b2d1e6a444ae71cea7c82a98ab5571c10da425a8c8adc4b33c22872ed67fa4`.

UDP discovery returned `bootnx` at 14:43:58 UTC. Nxlink transferred
26,732,897 compressed bytes / 2,247 blocks with exit 0 from 14:44:13 to
14:44:46 UTC. No TCP preflight consumed the netloader listener.

USB/MTP retrieval at 14:47:30 UTC supplied **32 reports / 541,985 bytes**.
Every size, SHA-256, baseline difference and raw ZIP member byte/CRC was
independently verified. Thirteen reports changed; nineteen match the preceding
sphere run. Source timestamps were unavailable, so identical reports cannot
independently date an invocation. Dependency pins and the original nine-file
submodule patch/mtimes are preserved. NROs, raw reports and archives stay private.

## Accepted input scope

The new PADRead report records `read-pass`, buffer `0x9025F1B0`, 48 bytes,
**connected port 0** and rumble capability mask zero. Discovery records PADRead
at 617031, then PADClampCircle2 `0x801AE7DC` at 617044 with the same buffer and
argument 2. ClampStick `0x801AE5D8` and later input functions execute before
the new WPADProbe boundary. This establishes PADRead returned; it does not
constitute a per-button hardware test or capture the actual 48-byte array.

The next stop has channel r3=0, output type pointer r4=`0x80398F40`, stack
`0x80398F38` and LR `0x8051EEC0`. Both the pinned HLE override and public RMCP01
symbol map at `94585b8a8fd7a2a52f30640ccff316e57880b6c1` identify WPADProbe.
The Switch controller belongs to the PAD path. Without a Wiimote backend,
WPADProbe must preserve the pinned absent-remote branch, rather than report
that controller as a fabricated Bluetooth remote.

## Rendering and liveness limits

The snapshot still records 3,937 guest FIFO writes, 168 GXBegin hits,
103 GXFlush hits, **99 preceding successful presents / zero failures** and
**zero display-list replay calls**. These are earlier snapshots, not proof of
rendered content after PADRead. Sphere/display-list reports match the baseline;
discovery still reaches the sphere constructor and later input path.

The watchdog has 104 ACTIVE samples and one recovered STALE sample; maximum
interval is 1,413 ms and the final sample is ACTIVE. Guest current/running
thread `0x80347498` is coherent and the FST remains structurally valid. The UI
font archive now has a fresh decode-pass: 499,251 bytes consumed and
3,153,052 bytes produced. Decoding does not establish visible font rendering.
Dispatch totals and elapsed times vary with scheduling; lower totals than the
preceding run are not a regression or a performance measurement.

WPADProbe and subsequent SDK boundaries, actual replay, recognizable images,
sustained execution, full input/audio behavior and performance remain open.
The [preceding result](HARDWARE_RESULTS_2026-10-04_SPHERE_PAD_READ_FRONTIER.md)
and [PADRead implementation](PAD_READ_2026-10-04.md) retain their own scope.
