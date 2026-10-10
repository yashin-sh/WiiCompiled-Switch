# DrawQuad return and IA8 repeat stop — 2026-10-10

The checked quad candidate `380fbc4` launches with nxlink exit 0 at
08:38:11.950974 UTC. The NRO is 74,436,664 bytes, SHA-256
`a22d8b6a98af8492239acb0c8f545288bb72ba6fc1d4f544addf25f4ee98c9be`.
Its complete SD readback and prelaunch private/CI gates are verified.

USB retrieval preserves 39 reports / 840,193 bytes. Every report is read a
second time independently and compared by size/SHA; the complete SD candidate
is reread and matches. Twelve reports change relative to the S10 baseline;
27 retain their earlier bytes and are not treated as fresh evidence. The
operator sees the Wiimote warning, then a black screen and a crash.

Discovery records entry to `DrawQuad` at dispatch 652910, target `0x80084D20`,
LR `0x8007B294`, stack `0x80398F58`, fiber `0x80347498`. The arguments are
position `0x80398F60`, size `0x92133DD8`, one UV array at `0x92133E8C`, absent
colors and alpha `0xFF`. A later durable texture-load boundary occurs at
dispatch 652989, 79 dispatches after that entry, with the material caller's
inner stack `0x80398E38` and LR `0x800815F8`.

The checked quad bridge cannot invoke guest dispatch. The checked outer
caller invokes DrawQuad, restores its stack and returns; the material caller
then reaches its texture-load boundary. This accepts the observed quad call's
return through caller progression. It is not an individually logged return
event and does not accept every quad layout, new game pixels or playability.

The terminal guard is `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, target
`0x80170F2C`, at 58.259 seconds. The readable object `0x80398EB8` has words:

```text
00000195 00000000 00307C1F 0088A795
00000000 00000003 00000000 00400202
```

These encode a 32 × 32 IA8 texture at physical MEM2 `0x1114F2A0`, linear
filters, repeat S/T, disabled edge LOD, zero LOD/bias and no mipmaps, user data
or TLUT. The tile count is 64; the complete backing span is 2,048 bytes.
The existing address-independent family accepts I4/RGB5A3 with clamp and
therefore refuses this IA8/repeat descriptor before native publication.
The [IA8 correction](GX_IA8_REPEAT_2026-10-10.md) covers this boundary.

The fresh graphics report contains no Dawn uncaptured error and there is no
exception report. Six uncontrolled presentation windows cover 102 frames in
52,178 ms, approximately 1.955 Hz weighted; individual windows range from
0.170 to 7.869 Hz. The sampled preterminal snapshot at dispatch 650553 records
103 successful presentations and zero failures, a valid FST and matching
current/running fiber. These counters are not a controlled performance test.

FIFO capture and frame dumping are disabled with new run IDs; no new PNG or
replay is claimed. Game inputs, NROs, translated callers and raw diagnostics
remain private.
