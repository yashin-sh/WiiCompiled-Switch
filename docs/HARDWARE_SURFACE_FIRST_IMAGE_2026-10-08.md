# First actual Switch GPU surface — 2026-10-08

This is the preceding partial trial. The [corrected trial](HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md)
subsequently validates latest-image replacement and the stop checkpoint.

The PR #335 diagnostic produced a complete, independently decoded first-frame
PNG from the Aurora/NVK presentation surface: 1280×720, uniform RGBA `(0,0,0,255)`.
This establishes actual opaque black GPU pixels for that completed frame.
Recognizable game rendering and playability remain unproven.

The image workflow is only partially validated. At completed frame 30,
`frame-dump-status.txt` reports `FAILED`, with `cannot save complete image`.
Both PNG files still contain frame 1. The frame-30 in-memory statistics report
zero nonblack pixels and all pixels nonopaque; those statistics describe a later
readback, not the retained PNG. The latest-image checkpoint therefore failed.

The original save used rename onto an existing filename. A regression test that
refuses existing rename destinations reproduces the failed checkpoint. The
correction keeps the previous image in a sibling backup while installing the
complete temporary PNG, restoring it if installation fails. It also reports the
failed save stage. A new console trial is required before accepting latest-image
updates or the stop checkpoint.

The same run reaches the next guarded I4 load at object `0x80397F00`, slot 0,
36×32, source `0x109C1780`, caller LR `0x800C45D0`. The verified pinned caller
unconditionally performs the prior load at LR `0x800C45A8`, the draw helper at
LR `0x800C45C4`, then this next load. The coherent captured stack and runtime
support native/helper return for the prior object `0x80397EC0` through that
caller inference. The next object remains refused; this image correction does
not broaden texture admission.

The fresh run retains coherent main/FST/fiber state and 102 successful presents
without present failures. The initial FIFO capture is byte-identical to the
preceding black desktop replay. The later partially recorded Mii frame was not
submitted or captured. Raw reports, image files and product identity stay local;
the candidate, successful launch and retrieval manifest are bound privately.
