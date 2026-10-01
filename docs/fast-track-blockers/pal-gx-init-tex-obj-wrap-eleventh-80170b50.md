# PAL eleventh GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-10-01 real-Switch run records a successful thirteenth
`GXInitTexObjLOD` on `obj=0x908FAE00` and immediately reaches:

```text
obj=0x908FAE00
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The required post-LOD descriptor is exact:

```text
word2=0x0000FC3F
word3=0x00845FB5
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

Matching init: data `0x908BF6A0`, 64x64, format 0, repeat/repeat, no mipmap.

The bridge adds only this exact clamp/clamp tuple.
