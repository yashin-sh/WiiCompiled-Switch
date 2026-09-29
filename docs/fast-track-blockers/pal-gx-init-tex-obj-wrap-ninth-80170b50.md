# PAL ninth GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-29 real-Switch run records a successful eleventh
`GXInitTexObjLOD` on `obj=0x908FA840` and immediately reaches:

```text
obj=0x908FA840
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The matching post-LOD descriptor comes from the exact format-2 eleventh LOD:
`word2=0x0020FC3F`, `word3=0x00845D15`, `word5=0x00000002`,
`word7=0x00800202`.

The bridge adds only this exact clamp/clamp tuple.
