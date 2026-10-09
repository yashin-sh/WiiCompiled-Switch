# Recognizable Switch image and desktop replay — 2026-10-09

The capture-enabled console trial of PR #341 code `9cc0ccf` preserves a
recognizable Wiimote safety page at presentation 4. The same FIFO prefix renders
that page on desktop. This establishes actual boot-screen pixels through
Aurora → Dawn → Vulkan/NVK on Switch. Menu progression, gameplay and steady
performance remain unproven.

## Image and replay evidence

Both controllers have fresh run identifiers compared with the preceding
capture-disabled trial. The 38 retrieved reports contain 21 changed files and
17 retained files; the previous terminal blocker is absent. The image status
identifies presentation 4 as the first nonblack image in both stages:

| Captured stage | Dimensions | Nonblack RGB pixels | Nonopaque pixels |
| --- | --- | --- | --- |
| Selected display copy | 655×480 | 284,254 | 314,400 |
| Presented NVK surface | 1280×720 | 834,175 | 0 |

The images were decoded with PNG CRC and row-size checks and inspected locally.
They show the Wiimote safety page during its dark opening fade. The presentation
path uses selected-XFB RGB and opaque output alpha, so transparent copy alpha
alone does not mean an invisible final image. This is the first retained,
recognizable boot image; earlier black checkpoints did not capture that interval.

FIFO recording remains within its budget and saves a complete 90-frame prefix
from GXInit. The live status reports 98 completed frames; the retained periodic
checkpoint ends at 90. The image status is `READBACK_COMPLETE`, rather than a
final `COMPLETE` checkpoint, and also identifies the latest PNG as frame 90.
Frames 91–98 are not attributed to the saved replay or latest PNG.

The desktop executable validates and replays all 90 frames with the same Aurora
code and Dawn/Vulkan on lavapipe. A private derivative prefix stops at completed
frame 4, replacing its frame boundary with the checked End record. Replaying it
preserves all preceding GX and memory state and reproduces the safety page.
At the selected-copy dimensions, 1,115 of 943,200 RGB channels differ from the
Switch copy, with maximum absolute difference 9/255 and mean 0.00186/255.
This is close agreement, not bit-exact agreement. Desktop output alpha is opaque;
the captured display copy has zero alpha. At frame 90 both outputs have exactly
black RGB. The surface PNG at that checkpoint is opaque black, and the EFB dump
is explicitly after the copy/clear operation.

## Progress and performance limits

The new KD bridge completes the earlier handle-2003 resume and close sequence.
The later handle-2004 post-resume probe that stopped the preceding trial is not
reached in the retrieved reports. No fresh terminal blocker or exception is
recorded. Absence of the old blocker therefore does not accept the new KD
transition on hardware.

With GPU readback enabled, the first four windows range from 3.10 to 4.47 Hz.
A later seven-presentation window takes about 104 seconds. All seven windows
average 0.678 Hz over 143.024 seconds, including loading, readback and stalls;
these are presentation rates, not gameplay FPS or a controlled comparison with
capture-disabled runs. The next trial uses the same validated candidate with
the SD capture-disable marker verified, to advance the pending KD path.

Old project NRO copies were backed up and verified before removal. After the
previous forty removals, three more prior copies/transfer aliases were archived,
leaving the current project candidate alongside unrelated homebrew. Future
Netloader transfers use the candidate's SD basename to avoid recreating the old
alias. Raw reports, game images, FIFO files, recovery archives and private NROs
remain local and excluded from the public repository.

The subsequent [capture-disabled trial](HARDWARE_KD_PROJECTION_2026-10-09.md)
accepts the formerly blocking KD post-resume probe and close, then reaches the
projection-vector getter. It does not attribute new image or replay files.
