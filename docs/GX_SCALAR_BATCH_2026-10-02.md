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

## Local validation result

Code candidate: `1406039e9addbeeb7889002faf96cb870934fba3`.
The following local gates pass:

- rendered and non-rendered executable contracts with ASan/UBSan;
- all rendered HLE branches and Discovery diagnostics syntax-compiled with
  devkitA64 and pinned WiiCompiled/Aurora headers;
- lint/format checks and repository tool self-tests;
- 25 build/verification steps from the four build workflows, including
  retention of all three bridges in the synthetic fast-track ELF;
- the private rendered Discovery CMake target, using the previously validated
  prepared Dawn/Aurora/NVK tree, with the pins and build command recorded locally;
- the final rendered ELF contains all three bridges and the real Aurora
  GXSetClipMode, GXSetDither and GXSetDstAlpha symbols.

GitHub Actions itself was not run. The original nine-file WiiCompiled working
patch was preserved byte-for-byte. No game-derived product or raw report was
committed.

The resulting `WiiCompiled-Switch-local-rendered-discovery-scan.nro` is
73,277,496 bytes, SHA-256
`cfa889d80b8e3a6b4435131142926fcc4801c2dd8b955940b5c73ca408108027`.
The static scan now reports 317 missing direct targets and 138 native targets.
These are coverage counts, not evidence of runtime progression.

Hardware validation of this batch is pending. The latest accepted hardware
frontier remains GXSetClipMode until this exact NRO produces new attributable
reports.
