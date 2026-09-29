# PAL seventh GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-29 rendered real-Switch run records a successful eighth
`GXInitTexObjLOD` on `obj=0x908FA820` and immediately reaches a new exact
wrap tuple:

```text
obj=0x908FA820
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The matching post-LOD descriptor is the already proven eighth LOD descriptor,
including `word2=0x0000FC3F` and `word3=0x00845EAD`.

The bridge adds only this exact clamp/clamp tuple. No neighboring wrap or
texture behavior is pre-ported.
