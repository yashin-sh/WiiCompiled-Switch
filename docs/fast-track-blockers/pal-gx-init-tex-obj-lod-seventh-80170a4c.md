# PAL seventh GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-28 rendered real-Switch run hardware-crosses the sixth exact
`GXInitTexObjLOD` descriptor on `obj=0x908FA5C0` and reaches a seventh
exact LOD descriptor:

```text
obj=0x907938A0
min_filter=1
mag_filter=1
min_lod=max_lod=lod_bias=0.0f
bias_clamp=0
edge_lod=0
max_aniso=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0083AC53
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The same run records the matching completed `GXInitTexObj` state for
`obj=0x907938A0`: data `0x90758A60`, 64x64, format 0, wrap 1/1, no mipmap.

The bridge adds only this exact descriptor to the LOD allowlist. No wrap tuple
for `0x907938A0` is added before hardware proves one.
