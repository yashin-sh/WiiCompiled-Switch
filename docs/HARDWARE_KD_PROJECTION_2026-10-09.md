# KD request accepted; projection-vector boundary — 2026-10-09

The capture-disabled run of PR #341 code `9cc0ccf` completes the formerly
blocking KD post-resume request. Its fresh ioctl report records
`cmd2-post-resume-probe-pass` on handle 2004, followed by a fresh successful close
on that handle and later translated/GX execution. This accepts that pending
reply and return sequence on Switch. Ready-phase repeats remain covered by
synthetic contracts and unproven on hardware.

Both capture controllers report `DISABLED` with identifiers distinct from the
[preceding image/replay trial](HARDWARE_NONBLACK_REPLAY_2026-10-09.md), and all
present-rate windows record `readback_enabled=0`. The 39 retrieved reports
contain 24 changed files, 14 retained files and one new blocker. Old SD PNGs and
FIFO prefixes are preserved but are not attributed to this run. The USB device
and readable DBI/MTP reports establish retrieval readiness; the operator has
not supplied a new visual observation.

## New diagnosed stop

The terminal report stops at `GXGetProjectionv`, PAL `0x801730CC`, after
60.862 seconds and 649,585 translated dispatches. The output pointer is a guest
stack range beginning at `0x80398F50`. The last sampled report records 102
successful presents, no failures, coherent running-fiber state and a valid FST.
The exception report is absent. The diagnostic stage label still names the
previous `GXSetDstAlpha`; the refused target identifies the actual next function.

The four measured presentation windows are:

| Presentations in window | Duration | Rate |
| --- | --- | --- |
| 43 | 5.000 s | 8.600 Hz |
| 41 | 7.161 s | 5.725 Hz |
| 10 | 18.741 s | 0.534 Hz |
| 5 | 6.890 s | 0.726 Hz |

Their aggregate is 2.620 Hz over 37.792 seconds. Loading and stalls remain;
these figures are neither gameplay FPS nor a controlled speedup benchmark.
The early rate is higher than in the capture-enabled trial, but the workloads
have different endpoints. Later pixels remain unknown with captures disabled.

## Shared projection state correction

Pinned WiiCompiled `runtime/src/hle/gx/gx_transform.cpp` maintains seven floats
alongside the native projection. The Switch matrix setter previously forwards
the matrix without retaining this shadow. The new getter therefore needs the
producer state as well as a seven-word output copy.

The correction adds the pinned default vector, updates it after every admitted
matrix setter, and exposes the getter and vector setter through the existing
CPU traits. Perspective matrices retain elements 0, 2, 5, 6, 10 and 11;
orthographic matrices retain 0, 3, 5, 7, 10 and 11. The getter writes the type
float followed by those six values as exact big-endian words, preserving CPU
state. The vector setter reconstructs the pinned native 4×4 matrix and retains
the original seven words after forwarding it to Aurora.

The local translated EGG state wrapper `0x802417FC` calls the getter
unconditionally and calls `GXSetProjectionv` / `0x80173080` when its comparison
requires an update. This supports implementing the save/restore pair together;
execution of the vector setter in this console run is not claimed. No fixed
stack address or particular matrix identity is admitted specially. The complete
28-byte vector range, or 64-byte matrix range, must be mapped without address
wrap before writes, state updates or native calls. Invalid ranges are diagnosed
before mutation. Headless vector operations stop explicitly.

A Nintendo-data-free contract invokes the actual traits and implementation with
the real Memory slice. It checks default state, perspective/orthographic matrix
selection, independently specified native matrices, noncanonical type values,
signed zero/NaN/infinity/subnormal payload preservation, exact output bytes,
complete-memory canaries and CPU preservation. Forked refusal checks cover short,
unmapped, zero and wrapping ranges and uninitialized memory, verifying unchanged
CPU, memory, shadow and native-call count. ASan/UBSan pass in rendered and headless
modes. Public CI runs this contract. The private rendered build, final provider
checks and the corrected projection pair's console trial remain pending.

The subsequent [projection trial](HARDWARE_PROJECTION_SCISSOR_2026-10-09.md)
accepts getter return through checked caller progression before the next
scissor-origin stop. The vector setter remains conditional and unproven on
hardware. Its private build, provider/retention checks and exact-head public CI
passed before that trial.
