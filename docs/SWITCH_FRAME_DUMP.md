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
- `frame-dump-status.txt`: unique run ID, readback frame, image-file frame,
  dimensions, row stride, nonblack RGB pixels, nonopaque pixels and uniformity.

The most recent completed images of all three stages are kept in bounded RAM.
Each saved stage has its own reported frame; a partial save marks capture FAILED.
A diagnosed stop saves that CPU snapshot without submitting the partially recorded
guest frame. `latest_png_frame` identifies the actual image on disk; it may lag
`frame` while execution continues. An abrupt native fault or power loss may leave
the last periodic image. A missing image or FAILED state never counts as success.

## Meaning and limits

`COMPLETE` means GPU mapping and complete PNG writing succeeded for the reported
completed frame. RGBA/BGRA channel order and 256-byte row padding are handled;
sRGB bytes and alpha are preserved. PNGs are uncompressed to avoid adding a
Switch codec dependency. Each dimension is bounded to 2048; there are five
retained image files. Replacement writes a complete temporary image, moves the
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
these runs are not performance measurements. The new paired stages and selected-
XFB presentation pass still require a console trial. The preceding [corrected console trial](HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md)
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
