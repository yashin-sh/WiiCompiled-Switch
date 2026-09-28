# PAL fourth GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-09-28 rendered real-Switch run hardware-crosses the fifth exact
`GXInitTexObjLOD` descriptor on `obj=0x908FA4E0`, then reaches:

```text
obj=0x908FA4E0
wrap_s=0
wrap_t=0
word0_before=0x00000195
word1=0x00000000
word2=0x0000FC3F
word3=0x00845FB5
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The Switch bridge adds only this exact clamp/clamp tuple to the existing
allowlist and reuses the already-proven pinned guest/Aurora mutation.
