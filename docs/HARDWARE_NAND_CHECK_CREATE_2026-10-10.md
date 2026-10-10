# NANDCheck return and NANDCreate frontier — 2026-10-10

The corrected NANDCheck candidate `b848972` launches through nxlink with exit
0 at 2026-10-10 17:18:59 CEST (15:18:59 UTC): 27,196,514 compressed bytes /
2,286 blocks. Its separate private rendered gate and all eight published
exact-source checks pass before merge and deployment. The complete SD NRO
readback verifies 74,436,664 bytes and SHA-256
`d7a39816163a65c8b03cbfbbdd21cfeb9cbdb30a861ea7fca5b8958131121af6`.
The older IA4 NRO is backed up, size/SHA verified and removed; the corrected
NANDCheck NRO is the only project candidate in the Switch directory.

After the operator reports “USB prêt, essai arrêté”, 39 reports / 851,184
bytes are retrieved and independently verified through a second USB copy.
Sixteen reports change and 23 are retained from the preceding trial. The
complete SD NRO is also reread. Raw diagnostics, local callers, binary
payloads and cleanup receipts remain private.

NANDCheck `0x8019EAD0` enters at dispatch 656688 with block size 184, count 4
and output `0x90230474`, LR `0x8052CA3C`, SP `0x90230438`, fiber
`0x902304E0`. The checked caller forwards these arguments and returns a
healthy wrapper result only after the native check succeeds. The next
caller `0x8052C68C` enters at dispatch 656689 on the same fiber, followed
by setup helper `0x8023AA04` at 656690 and creation helper `0x8023AA78` at
656692. These later entries retain stage `RMCP01_NAND_CHECK`.

This distinct stage and checked caller progression accept one scoped
NANDCheck return. The output word is not individually captured; acceptance
is a control-flow inference for this observed call, not an explicit native
return/output trace or arbitrary parameter-family hardware acceptance.

The new durable terminal is DIRECT NANDCreate `0x8019B43C` at dispatch
656692 / 60,365 ms: r3 `0x802581A3`, r4 permission `0x30`, r5 attributes 0,
LR `0x8052C6E4`, SP `0x90230368`, same fiber `0x902304E0`. The checked
creation helper forwards the path pointer from its title data and those
permissions/attributes. Original DOL attribution maps that pointer to the
NUL-terminated `/tmp/banner.bin`. This is static attribution: live path
bytes were not captured in this report. The [bounded creation bridge](NAND_CREATE_2026-10-10.md)
therefore checks every live byte, including the terminator, before accessing
the virtual NAND root. Later open/move operations remain separately guarded.

Captures are freshly disabled. The retained texture-load report is unchanged
and is not fresh proof of an individual texture load in this trial. No Dawn
uncaptured error appears in the fresh graphics report. Five uncontrolled
windows total about 1.92 Hz; this is neither a controlled performance
comparison nor steady gameplay. The operator provides no new screen detail
for this trial. Existing images are not attributed to this run, and menus,
gameplay, controls and fresh nonblack output remain unproven.
