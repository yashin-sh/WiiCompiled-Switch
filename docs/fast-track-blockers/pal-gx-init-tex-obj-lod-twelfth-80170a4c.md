# PAL twelfth exact GXInitTexObjLOD descriptor — `0x80170A4C`

The 2026-09-29 real-Switch run reaches a second exact descriptor shape on the
already-known `obj=0x9018E480`:

```text
word0 = 0x00000095
word1 = 0x00000000
word2 = 0x0020FC3F
word3 = 0x0080A6F7
word4 = 0x00000000
word5 = 0x00000002
word6 = 0x00000000
word7 = 0x00800202
```

LOD args remain linear/linear, zero min/max/bias, no bias clamp, no edge LOD,
GX_ANISO_1.

Matching init: data `0x9014DEE0`, 64x64, format 2, repeat/repeat, no mipmap.

The prior descriptor on the same object was format 0 and remains separately
allowlisted. The bridge adds only this new exact descriptor.
