# PAL thirteenth exact GXInitTexObjLOD descriptor — `0x80170A4C`

The 2026-09-30 real-Switch run first records the ninth wrap tuple on
`obj=0x908FA840` as `wrap-pass`, then reaches:

```text
obj      = 0x908FAE00
word0    = 0x00000095
word1    = 0x00000000
word2    = 0x0000FC3F
word3    = 0x00845FB5
word4    = 0x00000000
word5    = 0x00000000
word6    = 0x00000000
word7    = 0x00400102
```

LOD args remain linear/linear, zero min/max/bias, no bias clamp, no edge LOD,
GX_ANISO_1.

Matching init: data `0x908BF6A0`, 64x64, format 0, repeat/repeat, no mipmap.

These words match the earlier fifth descriptor on `obj=0x908FA4E0`, but the
object address is new. The bridge adds only the new exact object+descriptor
combination.
