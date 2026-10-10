# Fog returned; large IA4 descriptor boundary — 2026-10-10

The exact Fog-zero candidate `bb00520` launches through nxlink with exit 0
at 15:33:09 CEST (13:33:09.490611 UTC): 27,197,070 compressed bytes / 2,286
blocks. Its 74,436,664-byte NRO SHA-256 is
`225a1f44701ba7e9284409e714406f33b2aba7b6e0da95824c483a1c7281274b`.
Complete code-HEAD checks, private gate, NRO hash and unchanged code are
revalidated before transfer. The Switch is subsequently detected in USB/DBI;
no fresh operator screen observation is supplied.

All 39 reports / 842,870 bytes match independent second USB reads by size
and SHA, verified at 13:35:36.521067 UTC. Complete SD NRO readback also matches.
Twenty-one reports change from the preceding IA4 trial and eighteen retain
previous bytes. Retained files are not fresh evidence. FIFO capture and frame
dumping have fresh run IDs and remain disabled; no new image/replay is claimed.

Discovery reaches setup `0x805CF2BC` at dispatch 653036, then `0x805CF598` at
653072, LR `0x805CEE78`, stack `0x80398F48`, fiber `0x80347498`. The checked
outer caller invokes setup before that later drawing helper. Setup's surviving
paths call type-0 Fog with all four `.d` arguments assigned from the same f32.
Of the three complete tuples admitted by the executed bridge, only the
all-positive-zero tuple has all four values equal. The later helper and its
new durable texture guard therefore support one exact Fog-zero return through
checked caller progression. This is an inference, not an individually logged
native return, and does not accept other Fog tuples or paths.

One DrawQuad entry at dispatch 651335 precedes a distinct same-fiber target
`0x805E805C` at 651604. Checked immediate outer epilog and absence of guest
dispatch in the bridge support that one scoped quad return. Broader layouts
and the preceding exact texture tuples are not individually retained returning.

The terminal is `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, target `0x80170F2C`,
dispatch 653089, 54.669 seconds after the first translated dispatch. PC
`0x800060A4`, LR `0x805CF5F4`, stack `0x80398F38`, object `0x90D28D70`, slot 0:

```text
00000190 00000000 002FFFFF 00855397
00000000 00000002 00000000 00000202
```

This is 1024 × 1024 IA4/clamp at physical source `0x10AA72E0`, linear filters,
disabled edge LOD, zero LOD/bias, no mipmaps/user data/TLUT. The pinned guest
initializer stores only `(tiles & 0x7FFF)` in word7: 32,768 tiles encode as
zero, while the full backing span is 32,768 × 32 = 1,048,576 bytes. The current
validator refuses this count. The [bounded correction](GX_IA4_LARGE_2026-10-10.md)
checks the masked field and preserves full dimension-derived range validation.
The checked load caller reads the texture object from its draw record and
calls map 0 when that identity differs from its previous one; no unlogged
record contents or game texture pixels are inferred.

No Dawn uncaptured error appears in the changed graphics report, and no
exception report is present among the retrieved project files. Five
uncontrolled windows cover 102 frames / 47,794 ms, about 2.134 Hz weighted,
ranging from 0.161 to 8.754 Hz. These sampled windows are not a controlled
performance comparison. Fresh game pixels and playability remain unproven.
Private game data, callers, NROs and raw diagnostics stay excluded.
