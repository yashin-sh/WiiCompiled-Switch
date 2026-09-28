# PAL sixth GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-28 rendered real-Switch run records a successful sixth
`GXInitTexObjLOD` on `obj=0x908FA5C0` and immediately reaches a new
unproven wrap tuple for that same object:

```text
obj=0x908FA5C0
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The matching post-LOD descriptor is the already proven sixth LOD descriptor,
including `word2=0x0000FC3F` and `word3=0x00845FBC`.

The bridge adds only this exact clamp/clamp tuple. The fifth wrap on
`0x907938A0` and eighth LOD on `0x908FA820` remain separate
scheduler-dependent pending hardware gates.
