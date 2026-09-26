# PAL GXInitTexObjWrapMode first-observed blocker — `0x80170B50`

The 2026-09-26 rendered real-Switch run durably crosses the exact first
`GXInitTexObjLOD` tuple and stops at:

```text
target = 0x80170B50
r3     = 0x9018E120
r4     = 0
r5     = 0
```

`doldecomp/mkw` maps this address exactly to
`GXInitTexObjWrapMode` (`0x80170B50..0x80170B64`).

The immediately preceding LOD pass proves:

```text
obj         = 0x9018E120
guest_word0 = 0x00000195
guest_word1 = 0x00000000
```

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` updates only the low wrap bits
of guest word0, then forwards the wrap modes to Aurora. For clamp/clamp:

```text
0x00000195 -> 0x00000190
```

Implement only this exact first tuple and exact post-LOD pre-state.

Do not pre-port neighboring texture-object helpers.

Crossing proof requires a later durable hardware frontier beyond this call.
