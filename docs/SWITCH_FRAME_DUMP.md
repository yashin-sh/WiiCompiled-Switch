# Actual Switch surface images

This opt-in diagnostic reads the selected XFB/display copy, the final NVK
surface, and the persistent EFB after the copy/optional clear. The renderer
presents the selected display copy rather than displaying the cleared EFB.
It is an image readback from the console, separate from replaying FIFO on desktop.
The surface includes presentation scaling; it is copied after rendering and
before Present releases it. A separate submission keeps a failed diagnostic copy
from invalidating the already submitted render commands.

## Build and collect

```sh
MKW_RENDERED_FRAME_DUMP=ON MKW_DISCOVERY_SCAN_MODE=ON \
  MKW_RENDERED_FIFO_CAPTURE=ON MKW_JOBS=4 \
  bash scripts/build-local-rendered-fast-track.sh
```

The local output is `WiiCompiled-Switch-local-rendered-frame-dump.nro`.
The option defaults to OFF and controls only readbacks and SD saves. The
persistent EFB and selected-XFB presentation pass are used by the normal
rendered build too.
On the SD, `switch/WiiCompiled-Switch/` contains:

- `surface-first.png`: first successfully presented surface read from the GPU.
- `surface-latest.png`: last saved completed surface, refreshed every 30 presents
  and at a diagnosed unsupported dispatch or normal renderer shutdown.
- `display-copy-first.png` / `display-copy-latest.png`: the selected XFB copy
  before surface scaling/opaque presentation alpha.
- `efb-after-copy-latest.png`: EFB after the optional clear, explicitly not a
  pre-clear EFB snapshot. A cleared EFB can coexist with a valid colored XFB.
- `surface-first-nonblack.png` / `display-copy-first-nonblack.png`: first completed
  RGB content in each stage, retained even if it occurs between periodic saves.
- `frame-dump-status.txt`: unique run ID, readback frame, image-file frame,
  dimensions, row stride, nonblack RGB pixels, nonopaque pixels and uniformity.

The most recent completed images of all three stages are kept in bounded RAM.
Each saved stage has its own reported frame; a partial save marks capture FAILED.
A diagnosed stop saves that CPU snapshot without submitting the partially recorded
guest frame. `latest_png_frame` identifies the actual image on disk; it may lag
`frame` while execution continues. An abrupt native fault or power loss may leave
the last periodic image. A missing image or FAILED state never counts as success.

## Compare capture cost

Create `switch/WiiCompiled-Switch/render-captures-disabled.flag` on SD before
launching the diagnostic NRO. Both controllers write a fresh `DISABLED` status,
keep earlier files intact and skip GPU readbacks/FIFO snapshots. Remove the
marker before a diagnostic launch to restore capture. It is sampled only at
startup, so the same renderer can be compared without rebuilding generated code.

`rendered-fast-track-graphics.txt` reports completed-present frequency over
windows of at least five seconds, with frame count, elapsed host milliseconds
and current readback state. No extra GPU wait is added. Loading, guest work and
stalls are included; these rates are not a steady-state gameplay benchmark.
Keep Discovery unchanged for the initial comparison. Performance with captures
disabled is pending hardware measurement.

## Meaning and limits

`COMPLETE` means GPU mapping and complete PNG writing succeeded for the reported
completed frame. RGBA/BGRA channel order and 256-byte row padding are handled;
sRGB bytes and alpha are preserved. PNGs are uncompressed to avoid adding a
Switch codec dependency. Each dimension is bounded to 2048; there are five base
image files and up to two additional first-nonblack files. Replacement writes a complete temporary image, moves the
previous image to a `.previous` backup, installs the new image and removes the
backup. A failed installation restores the old image; if restoration itself
fails, that backup is retained and capture fails. This accommodates SD rename
semantics that refuse an existing destination. Power loss during replacement may
leave a backup rather than the final filename; it never counts as a complete save.
The three readbacks finish in reverse encode order to pop nested device error
scopes correctly. Each validation/map wait has a one-second limit;
failure disables further readbacks while retaining any labelled last good image.

A black result is evidence about that completed surface. A nonblack result
shows actual RGB content but does not establish that the game image is correct.
Neither captures the later partially recorded Mii scene or proves physical
scanout, input, sound or playability. Readbacks and SD writes add diagnostic cost;
these runs are not performance measurements. The [paired-stage trial](HARDWARE_CAPTURE_CONTROL_2026-10-09.md) records black
frame-1/90 pixels with opaque final surface alpha, while the operator reports
boot images/low FPS absent from those retained files. The live status reached
102 before manual exit without a final save. The subsequent
[first-nonblack trial](HARDWARE_NONBLACK_REPLAY_2026-10-09.md) retains the recognizable
Wiimote safety page at frame 4 in both selected-copy and presented-surface PNGs.
Its live status reaches 98, while the retained periodic PNG/replay checkpoint
ends at 90 and remains black. The first-nonblack retention is now hardware-proven;
the pending KD post-resume request and later rendering still need another run. The preceding [corrected console trial](HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md)
validates first/latest image saving and a checkpoint at frame 102. Both images
have black RGB; the first has alpha 255 and the last alpha 0. The retained alpha
is reported separately from RGB and does not establish the cause of black output.
The [preceding trial](HARDWARE_SURFACE_FIRST_IMAGE_2026-10-08.md) failed when
replacing the latest PNG; that SD replacement failure is resolved.
Game-derived images and raw diagnostics remain local and excluded from Git/CI.

## Public reproduction

```sh
bash scripts/test-frame-dump.sh
bash scripts/build-desktop-gx-replay.sh
bash scripts/test-desktop-gx-replay.sh
```

The portable test checks PNG CRC/zlib decoding with an independent decoder,
padded RGBA/BGRA rows, alpha/black statistics, SD replacement/rollback failures and first/latest
checkpoint lifecycle under sanitizers. The real Vulkan test exercises the same
readback helper on fabricated RGBA/BGRA/sRGB textures and a GPU clear, then rejects
an invalid GPU copy. The seven Aurora replay workloads use that helper and retain
their exact pixel oracles. No game dump is required for these public tests.
