# PAL fourth GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-27 rendered real-Switch run hardware-crosses both
`AIStartDMA (0x80124048)` and the third exact
`GXInitTexObjWrapMode` tuple on `obj=0x9018E140`, then reaches a fourth
exact LOD descriptor:

```text
obj=0x9018E480
min_filter=1
mag_filter=1
min_lod=max_lod=lod_bias=0.0f
bias_clamp=0
edge_lod=0
max_aniso=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0080A890
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The fourth object differs from every prior proven LOD descriptor by object
address and descriptor content. The bridge therefore adds only this exact
tuple to the allowlist.

The pinned mutation remains the same already-proven linear/linear zero-LOD
operation. No neighboring GX texture API and no future wrap tuple for
`0x9018E480` is added before hardware reaches it.
