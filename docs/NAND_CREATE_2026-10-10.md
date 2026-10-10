# Bounded virtual NANDCreate — 2026-10-10

The [NANDCheck hardware trial](HARDWARE_NAND_CHECK_CREATE_2026-10-10.md)
reaches DIRECT NANDCreate `0x8019B43C` with permission `0x30`, attributes 0
and path pointer `0x802581A3`. Static original DOL attribution identifies
`/tmp/banner.bin`; the preceding report does not capture its live bytes.

The source-owned native registry now admits this call only after validating
the complete 16-byte guest range and matching every live byte, including the
NUL terminator. Other paths, unreadable ranges and argument tuples produce
a durable diagnosed refusal before looking up or modifying virtual NAND.
Null CPU is a no-op. Common ABI headers and translated caller shards are
unchanged by this addition.

The host path is the application's SD-backed virtual NAND root followed by
`tmp/banner.bin`. It is not the host's global temporary directory. The pinned
runtime ignores permission/attribute host mapping, creates parent directories,
returns 0 for a newly created empty file, -6 when the destination already
exists and -64 for storage failure. The bridge preserves those results for
this bounded family, using exclusive creation to prevent an existing file
from being truncated, including concurrent creators. Only r3 changes;
guest memory and the rest of the CPU context are preserved. A private status
report records the live path words and result after the call.

`bash scripts/test-nand-create.sh` checks the original pinned NAND API and
filesystem source blobs at `a135beb201042b20f390c6695ca6b26768820fb4`.
It runs the actual dispatcher, registry, Memory slice and production bridge
against real isolated files and an extracted pinned native helper oracle,
with ASan, fatal UBSan and LeakSanitizer in both rendered modes. Each mode
passes 14 admitted calls and 84 diagnosed refusals, plus a two-process
exclusive creation race. Cases cover binary existing-file preservation,
directory destinations, blocked parent paths, injected access failure,
unaligned guest paths, every permission/attribute bit and path byte,
null/unmapped/truncated ranges and full CPU/guest snapshots.

Five private negative controls are assertion-rejected after successful
compilation: unknown-path admission, unknown-argument admission, truncating
creation, the wrong existing-file result and r4 clobbering. Related DrawQuad
and GX registry contracts also run. The published gx-contracts job includes NANDCreate.
Prior unchanged renderer/GPU and NANDCheck proofs keep their original source
provenance. This change does not establish arbitrary NAND path handling,
physical NAND access, later open/move semantics or gameplay.

Candidate `86b2ce5` passes the separate exact-source private rendered-build gate and all eight published checks in the complete paginated rollup. [PR #372](https://github.com/yashin-sh/WiiCompiled-Switch/pull/372) merges at 2026-10-10T16:04:08Z as `1ae1a11`. The 74,436,664-byte NRO passes complete SD byte/SHA readback. The capture-disabled marker is independently reread. Every older owned project NRO is backed up, size/SHA verified and removed, leaving only the current project candidate. The exact corrected NANDCreate NRO launches via nxlink with exit 0 at 2026-10-10 19:39:47 CEST (2026-10-10T17:39:47.375774+00:00): 27,199,850 compressed bytes / 2,287 blocks. Its SHA, complete successful code-HEAD rollup, private gate and unchanged non-documentation sources are revalidated before transfer. The [fresh USB result](HARDWARE_NAND_CREATE_WRITE_2026-10-10.md) verifies 40 reports / 851,403 bytes and accepts the observed creation with result 0, matched live path and later checked caller progression before NANDWrite. Fresh pixels/playability remain unproven.

The immutable offline build validates 72 rendered HLE source files, synthetic
input variants, the libnx backend, Discovery and opt-in capture paths, plus
the retained NANDCheck SDK probe. No generated caller shard is recompiled.
The scoped link scan finds 86 unique providers across
274 explicit inputs; the final ELF retains 104 strong functions
and three GX state objects. Nine private patch bytes and nanosecond mtimes
remain preserved. Unchanged GPU proofs retain their original provenance.
The private NRO SHA-256 is `0a879f851099ab32e8a5024f6976dfe6f4fad9422c5162fd7e19968a772653b9`.
Complete SD readback verifies `sdmc:/switch/WiiCompiled-Switch-nand-create-86b2ce5.nro` at
2026-10-10T16:04:22.624334+00:00. Private payloads, diagnostic archives,
mutation copies, NROs and cleanup receipts remain excluded.
