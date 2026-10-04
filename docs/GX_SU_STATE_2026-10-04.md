# GX pending SU texture state — 2026-10-04

The [latest console test](HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md)
arrived at BeginDisplayList with guest GXData dirty word `1`, then refused
before recording. The [display-list implementation](GX_DISPLAY_LIST_2026-10-03.md)
now handles that bit through pinned native SU texture size/bias emission and
guest shadow publication. Other guest dirty bits remain diagnosed refusals.
This change has not yet been accepted on the console.

## State ownership and ordering

The rendered port's GX setters and texture loads update Aurora's native GX
shadow state. Several guest shadow fields are only partial bookkeeping; they
are not a complete copy of the native structure. Running a translated SU
routine against those fields would not establish the native state actually
used by rendering. The native SU routine is therefore authoritative here.
The guest SU word offsets and BP bookkeeping are checked against the local
RMCP01 SDK caller and existing coordinate mirror. Generated code stays private.

For guest dirty bit 0, the transport validates matching guest/native manual
scale masks, legal active direct texture maps and at most four indirect stages
before emitting anything. It invokes the actual pinned `__GXSetSUTexRegs`,
even if an earlier native flush already consumed native dirty bit 0. It then
clears only that native bit, preserving other native pending work for normal
Begin/End flushing. It publishes the final SU S/T words only for coordinates
that the native routine emitted. Manual, null-map and disabled-coordinate
paths preserve their guest SU fields and BP marker.

After successful emission, the guest SDK BP halfword is cleared for updated
coordinates; Aurora's native `bpSent=1` is retained separately. The handled
guest dirty bit is cleared only after the SU work and mirror publication.
Other guest context bytes and CPU fields remain unchanged.

Begin performs this work after all recording/memory/save-flag checks but
before native Begin and before either context snapshot. SU commands enter the
live FIFO, outside the new list. The saved context contains the flushed state.
End performs the same work before native End; SU commands generated while
recording enter the bounded list, affect its padded return count and obey the
existing save-context policy. With saving enabled, End restores the flushed
Begin snapshot, including guest/native/HLE state and AlphaCompare validity.

Successful processing writes `fast-track-gx-su-flush.txt` with the guest GXData
address, before/after dirty markers, updated-coordinate mask, emitted shadow
words and `phase=begin` or `phase=end`. An invalid native reference or mismatched
manual mask reports `GX_DISPLAY_LIST_NATIVE_SU_STATE` before writes. Unknown
guest dirty bits retain `GX_DISPLAY_LIST_PENDING_STATE`.

## Executable validation

The [display-list contract](../scripts/test-gx-display-list.sh) now extracts the
actual pinned SU routines, dirty-state dispatcher and gen-mode emission, in
addition to Begin/End, FIFO recording/padding, Flush and AlphaCompare. VCD,
VAT and BP-mask dirty branches are assertion seams and are not claimed as
executed decoder proof. Allocation and normal decoder forwarding remain host
seams. ASan, fatal UBSan and LeakSanitizer stay enabled.

The final changed-contract run passes **632 rendered cases / 39 refusals**,
plus both headless refusals. It covers all eight coordinates, maps 0..7,
all legal S/T wrap modes, dimensions 1 and 1024, both save policies and both
Begin/End phases. Exact big-endian BP bytes, live-versus-recorded ordering,
rounded counts, CPU preservation and whole guest-context effects are checked.
Further fixtures cover four indirect layouts, odd/even direct stages, repeated
coordinates, manual/null/disabled paths and preservation of other native dirty
work. Validation refusals also check the live FIFO's bytes and cursor remain
unchanged, in addition to the existing memory/context checks.

All five exact-code GitHub workflows / six jobs, private NRO construction
and fresh provider checks pass as recorded below.
Hardware must still confirm Begin returned and establish the next executed
boundary. End, recording/replay correctness, indexed/matrix-index layouts,
later SDK calls and recognizable game pixels remain unaccepted. Earlier
dated hardware evidence retains its original scope.

## Completed validation — 2026-10-04

Runtime code: `4a7e48b5214f6abd0e15b5b75d62572ecec6bc8d`. All thirteen local workflow contracts
pass; the changed display-list contract passes 632 rendered cases / 39
refusals and both headless refusals. Six compiled mutants are rejected:
omitted SU emission, omitted guest S-shadow publication, omitted HLE or
AlphaCompare restoration, wrong return count and omitted matrix-index guard.
Rendered AArch64 syntax, formatting, ShellCheck, Actionlint and Ruff pass.

The exact private Rendered Discovery NRO built at `2026-10-04T09:44:29.954210+00:00`
in five incremental tasks, using the immutable image
`sha256:b79d1d41459f5596427bff78007bcd61a5b398ac0def8e623798335dc124712f`,
without network access, at `-j3`. It contains 73,470,008 bytes, SHA-256
`9bdc32fcd7a6e85a5b4fdbff0037afc807a344a486114652d542bfcdf8362088`. The previously tested NRO and
its hardware diagnostics remain preserved privately.

Fresh final link-graph ownership checks cover thirteen scoped names, including
the SU adapter and actual `__GXSetSUTexRegs`, across 226 host inputs, 19 Rust
archives and seven named image libraries. Each has one strong provider; the
SU adapter is in `display_list_transport.o` and native SU in `GXManage.o`. The
checked FIFO header is present in the actual native source mirror. This is a
scoped provider audit, not exhaustive ownership or rendered-image proof.

Dependency pins and the original nine-file integration patch's bytes and
modification timestamps remain preserved. Console return from this correction
and recognizable game pixels require fresh hardware evidence.

All five actual GitHub workflows / six jobs pass on this exact runtime code.
The [build-switch log](https://github.com/yashin-sh/WiiCompiled-Switch/actions/runs/37192872894)
confirms 632 rendered cases / 39 refusals and both headless refusals. No
console return or recognizable game image is attributed to this correction yet.

## Console connection attempt

After final CI validation, nxlink attempted the exact NRO at `2026-10-04T09:56:36.310605+00:00`.
It returned exit 1 (`Connection to 192.168.1.194 failed`) at
`2026-10-04T09:56:36.383785+00:00`. UDP discovery had no netloader reply.
This is a connection failure, not a run of the corrected program, and adds
no Begin/End, image or crash result. The latest accepted console diagnostic
remains the preceding pending-SU-state refusal. A fresh netloader connection
is required for the corrected NRO's hardware test.
