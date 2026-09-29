# PAL tenth GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-29 rendered real-Switch run reaches a new exact LOD descriptor:

```text
obj=0x909019C0
min_filter=1
mag_filter=1
min_lod=max_lod=lod_bias=0.0f
bias_clamp=0
edge_lod=0
max_aniso=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0084635C
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The matching completed `GXInitTexObj` state is data `0x908C6B80`, 64x64,
format 0, wrap 1/1, no mipmap.

The bridge adds only this exact descriptor. No wrap tuple for
`obj=0x909019C0` is added before hardware proves one.
