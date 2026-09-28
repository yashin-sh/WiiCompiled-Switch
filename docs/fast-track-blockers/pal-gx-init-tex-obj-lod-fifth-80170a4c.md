# PAL fifth GXInitTexObjLOD tuple — `0x80170A4C`

The 2026-09-28 rendered real-Switch run hardware-crosses
`nw4r::snd::SoundPlayer::SetVolume (0x800A35E0)` and reaches a fifth exact
LOD descriptor:

```text
obj=0x908FA4E0
min_filter=1
mag_filter=1
min_lod=max_lod=lod_bias=0.0f
bias_clamp=0
edge_lod=0
max_aniso=0
word0=0x00000095
word1=0x00000000
word2=0x0000FC3F
word3=0x00845FB5
word4=0x00000000
word5=0x00000000
word6=0x00000000
word7=0x00400102
```

The fifth object differs from every prior proven LOD descriptor by object
address and descriptor content. The bridge therefore adds only this exact
descriptor to the allowlist. No wrap tuple for `0x908FA4E0` is added before
hardware proves one.
