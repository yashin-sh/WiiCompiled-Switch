# PAL third GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-26 rendered real-Switch run crosses the previously supported
second LOD descriptor and reaches a third exact object:

```text
obj=0x9018E140
minFilter=1
magFilter=1
min/max/bias=0/0/0
biasClamp=0
edgeLod=0
maxAniso=0

word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x0080A998
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The preceding GXInitTexObj pass shows a 64x64 I4 repeat/repeat,
non-mipmapped object backed by `0x90153300`.

Public doldecomp/mkw maps the address to `GXInitTexObjLOD`. Pinned
WiiCompiled applies the same linear/linear zero-LOD call and mirrors the guest
descriptor, yielding:

```text
word0 0x00000095 -> 0x00000195
word1 0x00000000 -> 0x00000000
```

Add only this exact third descriptor alongside the two already proven ones.
Do not generalize the LOD bridge or pre-port neighboring texture APIs.

Crossing proof requires a later durable hardware frontier beyond this tuple.
