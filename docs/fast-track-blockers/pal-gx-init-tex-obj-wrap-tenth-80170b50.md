# PAL tenth GXInitTexObjWrapMode tuple — `0x80170B50`

The 2026-10-01 real-Switch run records the format-0 LOD descriptor on
`obj=0x9018E480` as `lod-pass` and immediately reaches:

```text
obj=0x9018E480
wrap_s=0
wrap_t=0
word0_before=0x00000195
guest_word0=0x00000195
guest_word1=0x00000000
```

The required post-LOD descriptor is the original format-0 state:

```text
word2=0x0000FC3F
word3=0x0080A890
word5=0x00000000
word7=0x00400102
```

The separately captured format-2 descriptor on the same object is not covered.
