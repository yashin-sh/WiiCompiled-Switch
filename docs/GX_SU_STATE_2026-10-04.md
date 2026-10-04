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

CI, private NRO construction and fresh provider checks are being completed.
Hardware must still confirm Begin returned and establish the next executed
boundary. End, recording/replay correctness, indexed/matrix-index layouts,
later SDK calls and recognizable game pixels remain unaccepted. Earlier
dated hardware evidence retains its original scope.
