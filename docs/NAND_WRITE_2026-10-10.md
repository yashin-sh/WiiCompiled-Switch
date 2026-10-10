# Bounded temporary NANDWrite — 2026-10-10

The [NANDCreate trial](HARDWARE_NAND_CREATE_WRITE_2026-10-10.md) returns
0 after checking the live temporary pathname, then reaches NANDWrite
`0x8019B884` with file-info `0x902302D0`, buffer `0x80AE9540` and length
`0x72A0` (29,344). The checked caller opens `/tmp/banner.bin` in mode 2
and enters its write branch only after successful NANDOpen. The report
does not individually capture fd, file-info bytes or buffer contents.

The source-owned native registry admits only this length, fully mapped
file-info/flag and buffer ranges, and an actual live handle owned by the
existing SD-backed NAND runtime. That handle must name the application's
virtual `tmp/banner.bin`, have mode 2 and ordinary open flag 1, and still
be at position zero. Unknown handles, paths, flags, modes, lengths or
positions produce a diagnosed refusal before file mutation. Null CPU is
a no-op. The new narrow runtime interface is included only by its two
source consumers; common translated ABI headers remain unchanged.

For an admitted call, write the actual live guest bytes through the owned
FILE stream under the existing handle mutex. Match the pinned NANDWrite
helper's `fwrite` byte count and unconditional `fflush`, whose result the
pinned helper ignores. Change only r3, preserving guest RAM and the rest of
the CPU context. Publish a distinct stage and private status containing the
live fd/mode/flag and count after result publication. A short write remains
a short write. The change does not truncate the destination or write other
virtual files. It does not establish the broader crash-safe open/close save
protocol or arbitrary NANDWrite families.

`bash scripts/test-nand-write.sh` verifies the original pinned NAND API and
filesystem blobs, extracts the native write helper, and runs the actual
registry/dispatcher, Memory slice, owned OpenSync/SeekSync/CloseSync handle
runtime and production write bridge against real isolated files. ASan,
fatal UBSan and LeakSanitizer pass in both modes: nine admitted calls and
311 real diagnosed aborts each. Fixtures cover unaligned pointers, binary
payloads, preserved trailing bytes, a real failing storage stream, every
open-flag value, every length bit, null/unmapped/truncated ranges, unknown
and closed descriptors, other modes/paths and append-position refusal.
Full CPU and allocation snapshots prove preservation outside r3. Existing
source-only NANDCreate/DrawQuad/GX registry contracts are rerun.

Nine private incorrect variants of path/mode/flag/position/length admission,
write/flush behavior, byte counts and register preservation compile and are
assertion-rejected. The published gx-contracts job includes the
new production contract. No private game bytes, NROs or raw archives are
published; unchanged graphics proofs retain their original source provenance.

Its exact private build, complete successful CI rollup and corrected console execution remain pending.
