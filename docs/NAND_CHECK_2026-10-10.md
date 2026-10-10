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

The exact candidate's separate private rendered build and complete successful
published check rollup are required before merge/deployment. Corrected
NANDCheck execution on Switch remains pending. Each deployment also backs up
and verifies older owned NROs before removing them after the current NRO's
complete SD readback, as required by AGENTS.md. Private payloads, callers,
NROs, mutation copies and diagnostic archives remain excluded.
