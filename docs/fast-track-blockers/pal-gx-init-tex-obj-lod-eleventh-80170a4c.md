# PAL eleventh GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-29 rendered real-Switch run hardware-crosses the seventh exact
`GXInitTexObjWrapMode` tuple on `obj=0x908FA820` and reaches a new exact
LOD descriptor:

```text
obj      = 0x908FA840
word0    = 0x00000095
word1    = 0x00000000
word2    = 0x0020FC3F
word3    = 0x00845D15
word4    = 0x00000000
word5    = 0x00000002
word6    = 0x00000000
word7    = 0x00800202

minFilter = 1
magFilter = 1
minLod    = 0.0
maxLod    = 0.0
lodBias   = 0.0
biasClamp = false
edgeLod   = false
maxAniso  = 0
```

Matching `GXInitTexObj`: data `0x908BA2A0`, 64x64, format 2,
wrap repeat/repeat, no mipmap.

The bridge adds only this exact descriptor. No wrap tuple for
`obj=0x908FA840` or neighboring texture behavior is pre-ported.
