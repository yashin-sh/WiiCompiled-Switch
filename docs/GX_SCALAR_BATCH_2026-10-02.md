# Audited scalar GX batch (2026-10-02)

The user authorized grouping simple GX bridges to reduce repeated rendered
builds and Switch tests. This candidate covers the observed GXSetClipMode
frontier plus two statically missing scalar setters. It does not return from
unknown calls or pre-port guest-memory/resource/scheduler behavior.

## Pinned semantic audit

WiiCompiled pin: `a135beb201042b20f390c6695ca6b26768820fb4`.
The wrappers are in `runtime/src/hle/gx/gx_pixel.cpp`; Aurora implementations
are in `aurora-main/lib/dolphin/gx/GXCull.cpp` and `GXPixel.cpp`.

| Boundary | PAL target | Pinned arguments | Real Aurora effects | Hardware evidence before this batch |
| --- | --- | --- | --- | --- |
| GXSetClipMode | `0x8017351C` | r3 cast to GXClipMode | Writes XF register 5, sets `bpSent=1` | Exact blocker, r3=0 (`GX_CLIP_ENABLE`) |
| GXSetDither | `0x80172930` | r3 cast to GXBool | Updates cached cmode0 bit 2, emits BP word, sets `bpSent=1` | Not reached in the baseline trace |
| GXSetDstAlpha | `0x8017295C` | r3 cast to GXBool, r4 narrowed to u8 | Updates cached cmode1 alpha bits 0..7 and enable bit 8, emits BP word, sets `bpSent=1` | Not reached in the baseline trace |

`TARGET_PC` makes pinned Aurora's GXBool a bool: nonzero guest values become
true, rather than being narrowed to a byte. Destination alpha keeps the low
eight bits. No wrapper reads guest memory, modifies guest CPU registers,
invokes a callback, or updates another WiiCompiled tracking global.
Rendered bridges call these actual Aurora functions, preserving their state
and FIFO work. Non-rendered builds retain diagnostic-only synthetic behavior.

Each bridge has a distinct stage: `RMCP01_GX_SET_CLIP_MODE`,
`RMCP01_GX_SET_DITHER`, and `RMCP01_GX_SET_DST_ALPHA`. As with the existing
bridges, a stage is published before the Aurora call; the stage alone cannot
prove completion.

## Validation and acceptance

`scripts/test-gx-scalar-batch.sh` executes the actual bridge/trait code with
the real pinned CpuContext and Aurora types. GX sinks check argument
conversion, alpha truncation, one call per invocation, stage publication
before forwarding, null handling, and preservation of the complete CPU
context. It covers rendered and non-rendered branches with ASan/UBSan.
These tests verify forwarding, not the real FIFO/backend.

The synthetic batch probe exercises all three direct-dispatch instantiations;
fast-track CI retains the probe and requires all bridge symbols in the ELF.
The existing rendered syntax gate and private Discovery build must also pass.

Baseline: candidate `9786b78a1a09b91f7db5d13b8de1ce03510d02ab`, NRO SHA-256
`d9eaea1699ac39139d551022cd3301ea3d0470e68f6df03b63d1dda78d21fb4c`.
Its durable result is recorded in
`HARDWARE_RESULTS_2026-10-02_DISCOVERY_GX_SET_CLIP_MODE_FRONTIER.md`.

The static scanner reported 320 missing direct targets before this batch.
Covering these three reduces that count by three; the count does not include
all indirect/argument-dependent problems and is not a count of guaranteed
future runtime blockers.

On the next run, attribute each reached member independently using the
Discovery trace plus a later distinct target or durable milestone. Unreached
members remain pre-ported. Compare scheduler identities, FST/DVD evidence,
FIFO/present counters and native exceptions against the baseline. GPU
presentation does not establish visual pixel correctness.
