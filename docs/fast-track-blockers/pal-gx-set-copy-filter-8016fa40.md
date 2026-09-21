# PAL GXSetCopyFilter — 0x8016FA40

Tracking: #117, #162

## Hardware blocker

```text
kind   : DIRECT
target : 0x8016fa40
r1     : 0x80399088
r2     : 0x8038efa0
r3     : 0x00000000
r4     : 0x802457fe
r5     : 0x00000001
r13    : 0x8038cc00
stage  : RMCP01_EGG_ASYNC_DISPLAY_END_RENDER
```

`AsyncDisplay endRender = 1` plus the distinct later target and stage prove
that the merged `0x8020FF9C` bridge is hardware-crossed.

## Pinned semantics

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` maps `0x8016FA40` to
`GXSetCopyFilter`:

```text
r3 = antialias GXBool
r4 = 24-byte sample-pattern guest pointer
r5 = vertical-filter GXBool
r6 = 7-byte vertical-filter guest pointer
```

Hardware captured r3/r4/r5 but the blocker format did not capture r6.
Do not infer it.

## Switch implementation

Read live r3..r6, copy exactly 24 and 7 guest bytes when their pointers are
non-zero, and call Aurora `GXSetCopyFilter`.

Extend durable blocker logging to include r6.

Do not pre-port `GXSetDispCopyGamma`, `GXCopyDisp`, or later copy/present
boundaries before hardware reaches them.
