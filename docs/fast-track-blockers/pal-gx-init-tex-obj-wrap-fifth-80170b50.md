# PAL fifth GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-28 rendered real-Switch run records a successful seventh
`GXInitTexObjLOD` on `obj=0x907938A0` and immediately reaches the first
unproven wrap tuple for that same object:

```text
obj=0x907938A0
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The matching post-LOD descriptor is the already proven seventh LOD descriptor,
including `word2=0x0000FC3F` and `word3=0x0083AC53`.

The bridge adds only this exact clamp/clamp tuple. No wrap tuple for the
separate eighth LOD object `0x908FA820` is added before hardware proves one.
