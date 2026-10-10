# IA8 candidate result and clamp boundary — 2026-10-10

Candidate `e2aa724` launches through nxlink with exit 0 at
2026-10-10 12:51:41 CEST (10:51:41.660925 UTC). Its 74,436,664-byte NRO has
SHA-256 `56b77d76afbe7c2c4348a809e515aacffd3107bd21ab125024a667990dab7086`.
Private rendered-build and complete published exact-HEAD gates pass before
launch. A full independent SD readback after the trial matches that NRO.

USB retrieval verifies 39 reports / 839,522 bytes against independent second
reads by size and SHA. Twenty-two reports change relative to the preceding
quad trial; seventeen retain earlier bytes and are not fresh evidence. The
operator confirms USB readiness without supplying a fresh screen observation.

Discovery records DrawQuad at dispatch 652758, target `0x80084D20`,
LR `0x8007B294`, stack `0x80398F58`, fiber `0x80347498`:
position `0x80398F60`, size `0x92133DF8`, one UV array at `0x92133EAC`,
absent colors and alpha `0xFF`. A later material load guard occurs 122
dispatches later at dispatch 652880, with inner stack `0x80398E38` and
LR `0x800815F8`. The checked production quad bridge cannot dispatch guest
calls, and its checked outer caller restores the stack and returns immediately
after DrawQuad. This supports that one observed quad return through caller
progression; it is not an individually logged return or proof of every layout.

The prior repeat descriptor is no longer the terminal tuple. However, the
fresh load status is overwritten by a clamp refusal, and Discovery does not
separately retain the repeat tuple. An individually identified IA8/repeat
return is therefore not accepted from these reports.

The terminal `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR` targets `0x80170F2C`
at 57.832 seconds. Object `0x80398EB8`, slot 0, is readable:

```text
00000190 00000000 00307C1F 0088A796
00000000 00000003 00000000 00400202
```

These words encode 32 × 32 IA8 at physical MEM2 `0x1114F2C0`, clamp S/T,
linear filtering, disabled edge LOD, zero LOD/bias, no mipmaps, user data or
TLUT, and 64 complete 32-byte tiles. The complete backing span is 2,048 bytes.
Fresh initialization and LOD reports agree with the terminal descriptor.
The current structural family permits IA8/repeat and refuses this clamp
combination. The [clamp correction](GX_IA8_CLAMP_2026-10-10.md) covers it.

The changed graphics report contains no Dawn uncaptured error; no exception
report appears among the retrieved files. Six uncontrolled presentation
windows cover 102 frames in 51,730 ms, about 1.972 Hz weighted, ranging from
0.074 to 8.730 Hz. This does not establish a controlled performance change.
Fresh FIFO/frame-dump status has new run IDs and remains disabled. No new
image or replay is claimed; later game pixels and playability remain unproven.
Private game data, translations, NROs and raw diagnostics stay unpublished.
