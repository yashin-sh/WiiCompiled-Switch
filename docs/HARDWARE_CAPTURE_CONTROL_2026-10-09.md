# Texture-family and selected-XFB console trial — 2026-10-09

The user reports visible boot images with very low frame rate after launching
PR #338 code `98fb6ff`. This is a direct operator observation, not a pixel or
FPS measurement. The program was manually exited to retrieve its files before
a final diagnosed-stop or normal-shutdown checkpoint.

## What the retained files establish

Independent PNG CRC/zlib/full-pixel decoding verifies the five saved images.
The first surface and display-copy files contain opaque black RGB. The periodic
latest files belong to frame 90; the live status had advanced to frame 102.
`READBACK_COMPLETE` with `latest_png_frame=90` is not a complete frame-102 save.
The displayed boot images reported by the user are not present in these files.

| Saved stage at frame 90 | Dimensions | Uniform RGBA |
| --- | --- | --- |
| Selected display copy | 655×480 | `(0,0,0,0)` |
| Final surface | 1280×720 | `(0,0,0,255)` |
| EFB after copy/clear | 1280×720 | `(0,0,0,0)` |

This supports opaque final presentation on the tested NVK path, including when
the selected source has zero alpha. These black files do not validate colored
scaling, recognizable game content or steady rendering performance. A transient
image between periodic saves can be absent from the retained files.

The texture-family bridge advances beyond the former guarded Mii identity.
The last load report is a successful I4 32×64 load at a later object, and the
recorded caller progresses into further Mii drawing with coherent main/FST/fiber
state. There are 102 successful presents and no present failures. The final
recorded dispatch is not by itself a diagnosed next blocker.

## Replay limit exposed by this run

FIFO capture becomes `INVALID` with `capture memory exceeds bound` after four
completed frames. Only the first prefix had reached SD (`saved_frame=1`); later
completed prefixes were still in RAM when capture failed. The retrieved first
and latest replay files therefore both contain one frame. Full desktop Playback
validation succeeds, and the replayed pixels match the first Switch display-copy
image exactly at 655×480: opaque black.

The previous implementation copied every registered range at each drain even
when its bytes were unchanged, consuming the fixed 8 MiB sequence budget quickly.
Failure also relied on a later checkpoint/shutdown to save the last good prefix.
Neither later textured replay nor the reported boot images are validated by this
one-frame replay.

## Follow-up control

The correction serializes memory only when its bytes change, keeps mutations
ordered, and saves the last complete prefix when capture fails. GPU diagnostics
retain the first nonblack surface and selected-copy images, including a transient
colored frame between periodic saves.

A startup SD marker disables both capture controllers without rebuilding the
renderer. This keeps selected-XFB presentation and texture semantics, while
removing GPU readbacks and FIFO snapshots. Five-second-or-longer completed-present
windows report the observed presentation frequency without adding GPU waits.
Those windows include loading and guest stalls; they are not a steady-state FPS
benchmark. The revised capture/control code and performance trial require a new
console run. Discovery remains enabled for that comparison.

The preceding code passed all six public workflows/seven jobs and its private
rendered/SDK/provider checks. Executed-code identity, SD verification, retrieved
reports and raw images/replay files are retained privately. No game-derived
artifacts or private product fingerprints are published.
