# IA8 clamp trial and IA4 repeat boundary — 2026-10-10

The exact IA8/clamp candidate `9fcb11e` launches with nxlink exit 0 at
2026-10-10 13:45:46 CEST (11:45:46.713123 UTC): 27,196,467 compressed bytes
and 2,286 blocks. Its 74,436,664-byte NRO has SHA-256
`dabb56f3d963a362df57726f80bfbcbac2774622499075ea1a73c7ed2d1f138b`.
The code-HEAD full published rollup, private gate and NRO/source hashes are
revalidated before launch; complete independent SD readback after the trial
matches the same candidate. The operator reports that the trial stopped and
USB is ready, without supplying a fresh visual observation.

All 39 retrieved reports / 841,601 bytes match independent second USB reads
by size and SHA. Twenty-two change from the preceding IA8 trial and seventeen
retain their previous bytes; retained files are not fresh execution evidence.

Discovery records DrawQuad at dispatch 652088, target `0x80084D20`,
LR `0x8007B294`, stack `0x80398F58`, fiber `0x80347498`: position
`0x80398F60`, size `0x92133DD8`, one UV array at `0x92133E8C`, absent colors,
alpha `0xFF`. Later distinct translated targets carry its stage on the same
fiber, beginning at dispatch 652381. Execution subsequently reaches layout
and text targets `0x8007D2F0`, `0x8007D1B0` and `0x8007D520` before the
new material load guard. The checked quad bridge cannot dispatch guest calls;
its checked outer caller restores the stack and returns immediately after
DrawQuad. This supports one observed quad return through caller progression,
not an individually logged return or acceptance of every quad layout.

The preceding IA8/clamp tuple is no longer the terminal guard. Final load/init
status is overwritten by the later IA4 descriptor, however, and that exact
IA8 tuple is not separately retained returning. An individually identified
IA8/clamp return is therefore not accepted from these reports.

The terminal `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR` occurs at dispatch 653798,
56.899 seconds, target `0x80170F2C`, LR `0x800815F8`, stack `0x80398E38`.
Object `0x80398EB8`, slot 0, is readable:

```text
00000195 00000000 00207C1F 008B73EF
00000000 00000002 00000000 00200202
```

This is 32 × 32 IA4 at physical MEM2 `0x116E7DE0`, repeat S/T, linear filters,
disabled edge LOD, zero LOD/bias, no mipmaps, user data or TLUT. IA4 uses 8 × 4,
32-byte tiles: 32 tiles require a complete 1,024-byte backing span. The
current structural family admits I4, IA8 and RGB5A3; format 2 is the new
refused boundary. The [IA4 correction](GX_IA4_2026-10-10.md) adds that format
with audited rectangular tile geometry.

The changed graphics report contains no Dawn uncaptured error, and no
exception report appears among the retrieved project files. Five uncontrolled
presentation windows cover 102 frames in 49,919 ms, about 2.043 Hz weighted,
with individual windows from 0.145 to 8.996 Hz. This is not a controlled
performance comparison. FIFO capture and frame dumping have fresh run IDs
and remain disabled. No new image, replay or playability result is claimed.
Game data, translated callers, NROs and raw diagnostic archives remain private.
