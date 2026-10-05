# PADReset returned; Mii texture Fog frontier — 2026-10-05

The validated [PADReset bridge](PAD_RESET_2026-10-05.md) returns for the observed
mask `0x70000000`. Execution continues to a new `GXSetFog` tuple at
`0x801722CC`: type 0, start/end/near/far `(1,1,0,0)`, readable RGBA `(0,0,0,0)`.
The durable stop is `GX_SET_FOG_UNPROVEN_ARGS`, dispatch **635434 / 129.411 seconds**.
The user confirms an **initial black screen followed by an error** for this run.

## Candidate and retrieval binding

The private Rendered Discovery NRO is 73,494,584 bytes, SHA-256
`edddfff6a81d3c42973d37379b80d3577571b992cd3ecbd95e238cf49f848a0d`, code
`7ac668e74d5cb4ec21d668c87f76a09b8d1b99e8`. Launch revision `1ce3c42` changes
only Markdown and is included in merged PR #314 (`3d20141`). Candidate source
hashes, pins, original upstream patch and NRO bytes/hash are reverified before
launch. Direct nxlink exits 0 at **2026-10-05 20:07:42 UTC**, sending
26,743,991 compressed bytes / 2,247 blocks. An earlier 18:20:45 UTC attempt
failed its network connection before an application launch was observed.

USB/MTP retrieval at **20:10:40 UTC** copies **36 reports / 629,680 bytes**.
Every report size/SHA-256, baseline difference and raw ZIP member is verified;
ZIP CRC checks pass. Twenty reports differ from the
[motor-run baseline](HARDWARE_RESULTS_2026-10-05_PAD_CONTROL_MOTOR_PAD_RESET_FRONTIER.md),
sixteen are byte-identical. An independent second SD read confirms the new
PADReset report. Source timestamps are unavailable; no runtime build ID or
raw guest outputs are captured. The binding uses the verified launch and new
reset/later-blocker sequence. Private products and raw archives remain excluded.

## Return and new refusal

Discovery records PADReset entry at dispatch **620723**, LR `0x80523848`,
stack `0x80399058`, mask `0x70000000`, coherent fiber `0x80347498`.
The new native report records `pinned-reset-pass`, return value 1 and
`controller_reset=none`. The later distinct Fog refusal establishes the
observed reset return. Other masks retain host proof; no physical reset is claimed.

The new Fog stop captures:

```text
target / kind     = 0x801722CC / GX_SET_FOG_UNPROVEN_ARGS
type / color ptr  = 0 / 0x80397B24
f1..f4 f64 bits   = 3FF0000000000000 / 3FF0000000000000 / 0000000000000000 / 0000000000000000
RGBA              = 00 / 00 / 00 / 00 (readable)
LR / r1           = 0x800C3394 / 0x80397B18
r2 / r13          = 0x8038EFA0 / 0x8038CC00
dispatch / ms     = 635434 / 129411
stage             = RMCP01_GX_SET_FOG
action            = abort after durable blocker record
```

Pinned WiiCompiled attributes the target to `GX__SetFog_801722cc`, forwarding
f1..f4 narrowed to float and the checked guest RGBA to Aurora. PAL symbols place
LR in `RFLiMakeTexture` (`0x800C2680..0x800C36F3`); this is caller-family
attribution to Mii texture preparation, without claiming an independently
verified exact callsite. Discovery retains only the first hit per address:
its earlier Fog entry at dispatch 607760 is the already accepted initial tuple,
not an argument capture of this later invocation.

The bridge deliberately refuses the new tuple before its own color lookup or
native call. The durable reporter independently captures the readable color.
Aurora explicitly handles equal start/end or near/far with coefficients
`A=0, B=0.5, C=0`. The [bounded follow-up](GX_FOG_DEGENERATE_2026-10-05.md)
adds this exact observed tuple; its new native return requires another run.

## Invariants and limits

The preceding heartbeat at dispatch **634961** has **5,988 FIFO writes**,
**102 successful presents / 0 failures**, zero display-list replay calls,
six TaskThread hits, valid FST and coherent fiber/current/running identities
`0x80347498`. Relative to the preceding motor-run snapshot, these are +2,051
FIFO writes and +3 successful presents. They are run snapshots, not an isolated
measurement of PADReset's rendering effect or evidence of recognizable pixels.

The new display-list report has `end-pass`, 32 bytes in a 64-byte buffer at
`0x921032C0`, saved context enabled. Replay remains unobserved.
The watchdog records **118 ACTIVE and two recovered STALE samples**, 120 samples
total, maximum interval 5,390 ms. Later ACTIVE progression excludes a persistent
stall in this path. Time is measured from the first translated dispatch.

Unchanged input reports can be retained files. This run accepts only the new
reset mask return and the documented later progression. New Fog return, physical
rumble/reset, per-button input, audio, recognizable pixels and sustained gameplay
remain open.
