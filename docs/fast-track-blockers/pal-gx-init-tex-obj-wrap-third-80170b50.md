# PAL third GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-26 rendered hardware run explicitly crosses the third
`GXInitTexObjLOD` descriptor on `obj=0x9018E140` and then reaches:

```text
obj=0x9018E140
wrapS=0
wrapT=0
word0_before=0x00000195
word1=0x00000000
word2=0x0000FC3F
word3=0x0080A998
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

Public doldecomp/mkw maps `0x80170B50` to `GXInitTexObjWrapMode`.

Pinned WiiCompiled
`a135beb201042b20f390c6695ca6b26768820fb4` applies:

```text
word0 = (word0 & ~0xF) | wrapS | (wrapT << 2)

0x00000195 -> 0x00000190
```

and forwards `GX_CLAMP/GX_CLAMP` to the existing host texture object.

Add only this third exact descriptor alongside the two already proven wrap
tuples.

Do not generalize wrap handling and do not pre-port neighboring GX texture
helpers.

Crossing proof requires a later durable hardware frontier beyond this tuple.
