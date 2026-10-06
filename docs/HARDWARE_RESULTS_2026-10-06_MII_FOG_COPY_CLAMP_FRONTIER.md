# Mii Fog returned; copy-clamp frontier — 2026-10-06

The [bounded Fog correction](GX_FOG_DEGENERATE_2026-10-05.md) passes the new Mii
texture tuple `(1,1,0,0)`. Execution continues through the existing pixel/copy
filter setters to DIRECT `GXSetCopyClamp` **`0x8016F618`, value 3**, dispatch
**631958 / 119.749 seconds**, LR `0x800C3394`, stack `0x80397B18`.
This run's visual observation is pending; recognizable game pixels remain unproven.

## Candidate and retrieval binding

The private Rendered Discovery NRO is 73,494,584 bytes, SHA-256
`7a0463aaf070422c1d4ae1191b22a0304616c1a88194eadb4ed389fcb5235a4f`, built code
`d2c8abe8f14aa6817e22b7a36edc0929d8dc08aa`. Launch revision `d9de36c` changes
only Markdown and is included in merged PR #315 (`f6da5a7`). All non-Markdown
source hashes, pins, original upstream patch and NRO bytes/hash are checked
before launch. Direct nxlink exits 0 at **2026-10-06 04:57:43 UTC**, sending
26,744,500 compressed bytes / 2,247 blocks.

USB/MTP retrieval at **05:00:28 UTC** copies **36 reports / 628,989 bytes**.
Every report size/SHA-256, baseline difference, raw ZIP member and ZIP CRC is
verified. Fourteen reports differ from the
[PADReset baseline](HARDWARE_RESULTS_2026-10-05_PAD_RESET_MII_FOG_FRONTIER.md);
twenty-two are byte-identical. Source timestamps, runtime build ID and raw guest
outputs are unavailable. Binding uses the verified launch and fresh caller/
later-blocker progression. Private NROs, raw reports and archives stay excluded.

## Fog return and caller proof

Discovery first records `RFLiSetupCopyTex` `0x800C2550` at dispatch **631945**:
LR `0x800C3394`, stack `0x80397B48`, r3 **5**, r4/r5 **128/128**, destination
`0x9210A740`, color pointer `0x80397B54`, fiber `0x80347498`.
Checked private generated code gives its 48-byte frame and calls Fog first,
setting type 0, zero RGBA at stack +12, f2=f1 and f4=f3.

The admitted initial Fog tuple has unequal pairs; it cannot satisfy those
assignments. Therefore, the later distinct clamp stop plus the checked caller
and whole-tuple guard establish the second admitted tuple returned. This is
control-flow/guard evidence, not a separately captured f64 tuple or native
return trace. Discovery retains only the first hit per address, so its earlier
Fog line describes the initial call, not this repeated invocation.

The caller then executes existing Dither(1), ColorUpdate(1), DstAlpha(0,0),
ZMode(1,3,1), BlendMode(1,0,0) and CopyFilter(0,0,0,0), before Clamp(3).
The durable blocker records stage `RMCP01_GX_SET_COPY_FILTER`, r4/r5/r6 zero,
r2 `0x8038EFA0`, r13 `0x8038CC00`, and abort after the record.
Clamp has arrived but has not returned in this run.

## Next bounded configuration

Pinned WiiCompiled `gx_stubs.cpp` forwards Clamp to Aurora, then best-effort
mirrors its two low bits into guest GXData offsets `0x23C` and `0x24C`.
Native Aurora updates only `copyClamp`, without FIFO writes.

The checked caller next requests TexCopySrc `(0,0,128,128)` at `0x8016F478`,
then TexCopyDst `(128,128,5,0)` at `0x8016F4DC`. These are forecasts from the
captured entry and checked caller, not observed calls. The
[bounded configuration candidate](GX_TEXTURE_COPY_CONFIG_2026-10-06.md)
includes these three audited setters and their exact guest/HLE state effects.
Actual texture copying, GPU/guest readback and neighboring APIs are not pre-ported.

## Invariants and limits

The preceding heartbeat at dispatch **631379** records **5,044 FIFO writes**,
**102 successful presents / 0 failures**, zero replay calls, six TaskThread hits,
valid FST and coherent fiber/current/running identities `0x80347498`.
The display-list End report passes with 32 bytes in a 64-byte buffer at
`0x921032E0`, saved context enabled. These snapshots precede the new clamp stop
and do not measure Fog's isolated rendering effects or prove recognizable pixels.

The watchdog has **114 ACTIVE and one recovered STALE sample**, 115 total,
maximum interval 1,858 ms. Later ACTIVE progression excludes a persistent stall
in the recorded path. Elapsed time is not a performance comparison.

Unchanged PADReset/input reports may be retained files. Acceptance is scoped to
the newly established Mii Fog return and documented caller progression.
Clamp/texture-configuration returns, replay, recognizable pixels, input/audio
and sustained gameplay remain open.
