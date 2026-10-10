# NANDWrite returned; NANDMove frontier — 2026-10-10

The PR #371 candidate `4dfd9805e892f4e9b7e38c4791ae12364fa699e8`
launches through the user-requested direct Netloader route at 21:40:22 CEST
with nxlink exit 0. Its 74,461,240-byte artifact has SHA-256
`bc04e4af6971401613fef094d771abff8a79ac33bd7e8c04a082bd3dd52936ac`.
The separate exact-source private rendered gate and all eight published
checks pass before code PR #371 merges. Launch-doc PR #377 also merges
with all eight exact-HEAD checks successful. This launch does not require
or claim a preceding new SD deployment.

After the operator returns to USB/DBI, 41 reports / 853,191 bytes are
retrieved and verified by a second independent USB copy. Nineteen reports
change and 22 are retained from the prior NANDCreate trial. The transmitted
Netloader artifact is independently hash-verified locally. No current screen
details are supplied; the operator reports readiness. Raw reports, game
payloads, generated callers and cleanup receipts remain private.

The fresh bounded creation status records the same fully checked live
`/tmp/banner.bin` path, permission `0x30`, attributes 0, and result -6:
the temporary file already exists. Checked caller `0x8023AA78` proceeds
on creation result 0 or -6. At dispatch 659694 it calls helper `0x8023AC3C`.
That helper reaches NANDWrite only after the temporary-file NANDOpen returns
0. The fresh write status records owned fd 101, mode 2, ordinary open flag 1,
file-info `0x902301F0`, buffer `0x80AE9540`, requested length `0x72A0`, and
actual result 29,344. The production guard verifies the complete buffer,
owned temporary-file handle and initial position before writing.

Write dispatch 659696 is followed by NANDClose `0x8019CA80` at 659697,
then NANDGetHomeDir `0x8019E40C` at 659698, on fiber `0x90230400`.
The checked helper calls close only when write returns the complete count,
and returns success to its parent only after close returns 0. The parent
calls NANDMove only after GetHomeDir returns 0. This accepts these observed
returns through the actual caller branches and fresh write status; dispatch
counts alone do not establish success. Two independent USB reads confirm
that the virtual `/tmp/banner.bin` contains 29,344 bytes. No comparison
against the original guest buffer or complete save acceptance is claimed.

The terminal is DIRECT NANDMove `0x8019BEE8`, dispatch 659699, elapsed
68,882 ms, PC `0x8024373C`, LR `0x8023AAE4`, SP `0x90230288`.
Its source pointer is `0x802581A3`; the destination-directory pointer is
`0x90230290`. The checked caller supplies the temporary path and the stack
buffer just populated by GetHomeDir. The pinned NANDMove appends the source
basename to the second argument; it is a directory rather than a complete
filename. Destination live bytes are not retained here. The
[bounded correction](NAND_MOVE_2026-10-10.md) must verify both complete live
paths before doing any virtual NAND operation.

Capture reports are freshly DISABLED with run id `1791662366539302504`.
The texture status is also fresh: 1024-square IA4/clamp, full 1 MiB load-pass.
No Dawn uncaptured error or exception report appears. Uncontrolled present
windows total 102 frames / 61,547 ms, about 1.66 Hz; this supplies no
controlled performance comparison or fresh image evidence. Menus, controls,
gameplay and complete save/recovery behavior remain unproven.

After report retrieval, the same candidate is copied to SD and completely
reread against its recorded size/SHA-256. The capture-disabled marker is
reread independently. The older NANDWrite NRO is backed up locally, checked
against its recorded size/SHA, removed, and the directory is verified to
contain only the current project candidate. This later SD receipt does not
change the earlier direct Netloader launch route.
