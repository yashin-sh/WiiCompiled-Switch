# NANDCheck virtual result contract — 2026-10-10

The [large IA4 trial](HARDWARE_IA4_LARGE_NAND_CHECK_2026-10-10.md)
accepts the observed full-size texture load, then reaches DIRECT NANDCheck
`0x8019EAD0` on the save-worker fiber. Captured arguments are blockSize 184,
blockCount 4 and output `0x9015B034`. Output contents are not captured.

The pinned WiiCompiled implementation ignores the first two parameters and
publishes one zero healthy-result word to a valid four-byte guest output.
It returns 0 after the write, or -8 for null/unmapped output or a Memory
write exception. Match this existing virtual NAND model without measuring
physical free space or enabling NAND write/create/delete operations.

The new source-owned `KnownNativeCpuCall<0x8019EAD0>` specialization is
included by the existing NAND trait header. It sets a distinct
`RMCP01_NAND_CHECK` stage, validates the complete output range before writing,
catches the pinned access exception and changes only r3 plus the four-byte
result. Null CPU is a no-op. Unknown native targets remain guarded.

`bash scripts/test-nand-check.sh` verifies both upstream source files against
Git blobs at `a135beb201042b20f390c6695ca6b26768820fb4`, then compiles the
actual production specialization with the real Memory slice under ASan,
fatal UBSan and LeakSanitizer in headless and rendered modes. Each mode passes
562 cases: the observed tuple, arbitrary ignored block parameters, every
unaligned output offset, truncated/unmapped/null ranges, the top of 32-bit
guest space, pre-init/post-reset calls and injected write exceptions. An
independent pinned helper oracle validates result/error/write behavior; full
CPU and allocation snapshots prove preservation beyond the allowed writes.

Five private incorrect variants are assertion-rejected after successful
compilation: omit the output write, clear eight bytes, admit a mapped null
pointer, clobber r4, and return the wrong error. Public sources are unchanged
by those runs. The new host contract is part of the published gx-contracts
check. Renderer/GPU inputs are unchanged; prior GPU proofs retain their
original provenance rather than being described as fresh NAND validation.

Candidate `b848972` passes the separate exact-source private rendered-build gate and all eight published checks in the complete paginated rollup. [PR #369](https://github.com/yashin-sh/WiiCompiled-Switch/pull/369) merges at 2026-10-10T15:11:35Z as `50618fd`. The 74,436,664-byte NRO passes full SD byte/SHA readback; the capture-disabled marker is independently reread. Every older owned project NRO is backed up, size/SHA verified and removed. The exact corrected NANDCheck NRO launches via nxlink with exit 0 at 2026-10-10 17:18:59 CEST (2026-10-10T15:18:59.653600+00:00): 27,196,514 compressed bytes / 2,286 blocks. Its SHA, complete successful code-HEAD rollup, private gate and unchanged non-documentation sources are revalidated before transfer. The [fresh USB result](HARDWARE_NAND_CHECK_CREATE_2026-10-10.md) verifies 39 reports / 851,184 bytes and accepts one observed NANDCheck return through checked caller progression before NANDCreate. The output word is not individually captured. Fresh pixels/playability remain unproven.

The offline immutable-image build recompiles the translated callers affected
by the NAND trait header and validates all rendered HLE SDK paths plus an
explicit NANDCheck specialization probe. The scoped link scan retains its
85 unique providers and 103 strong functions plus three GX state objects.
Nine original private patch bytes and nanosecond mtimes remain preserved.
The private NRO SHA-256 is `d7a39816163a65c8b03cbfbbdd21cfeb9cbdb30a861ea7fca5b8958131121af6`.
Its complete SD readback verifies `sdmc:/switch/WiiCompiled-Switch-nand-check-b848972.nro` at
2026-10-10T15:11:51.843781+00:00. Cleanup receipts, payloads, callers,
NROs, mutation copies and diagnostic archives remain excluded.
