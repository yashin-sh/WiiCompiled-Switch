# Bounded ordinary-read NANDGetLength — 2026-10-11

The [verified NANDSeek-candidate trial](HARDWARE_NAND_SEEK_LENGTH_2026-10-11.md)
stops earlier on the existing-file length branch at PAL `0x8019BF4C`.
Add a source-owned registry bridge for a live owned ordinary mode-1 stream,
complete 139-byte file-info span, complete four-byte output, and disjoint spans.
No fixed address, filename or expected length grants admission. Preserve the
opaque access byte; trust the host mode. Track safe origin on the owned handle,
including SafeOpenRead which has no shadow commit path, so forging an ordinary
guest flag cannot admit it. Stale/foreign handles, other modes, safe origins,
wrong flags, truncated or overlapping spans abort before any seek/tell call.

Match byte-verified pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4`: NandProbeFileExtent queries the
original position, attempts the end seek, queries the actual end, then attempts
to restore the original position. NANDGetLength writes that actual size cast to
u32 and returns 0 after a valid owned handle resolves. The pinned code ignores
seek return codes and does not normalize ftell -1. Keep this precise behavior,
including output 0xFFFFFFFF on a failed end query; report actual original
position and both seek results. Correct the shared backend to match the same
sequence for its existing frontend. Only r3 and the admitted four-byte output
change; no synthetic expected save size or forced stream position is used.

`bash scripts/test-nand-get-length.sh` links the actual dispatcher/registry,
production Memory and handle runtime, real binary files, and extracted
byte-verified NANDGetLength/NandProbeFileExtent bodies. Both rendered modes
pass 20 admitted calls and 415 genuine abort refusals with ASan, fatal UBSan
and LeakSanitizer active. Test unaligned and complete end-boundary objects and
outputs, zero/one/banner/0x2BC000-sized actual files, opaque bytes, both seek
failures and both tell failures, actual position effects, shared backend error
semantics, every ordinary flag alternative, stale fds, other modes, safe read
and shadow handles even after forged flags, and every overlapping output.
Complete CPU and guest snapshots prove only r3/output changes; refusals prove
an actual abort with no stdio operation. File bytes remain unchanged.

Required gx-contracts CI includes the new contract. Existing NAND, callback,
LYT and GX registry suites are rerun. Semantic negative controls, an exact-source
private rendered/Discovery/SDK build, scoped provider and retention audits,
and every check in the complete paginated exact-HEAD rollup must pass before
merge and SD/Netloader deployment. Corrected console execution is pending.
This does not accept the still-unreached rewind or complete save/recovery.
