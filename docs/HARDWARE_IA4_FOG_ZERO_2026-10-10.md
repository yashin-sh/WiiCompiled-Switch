# IA4 load returned; all-zero Fog boundary — 2026-10-10

Candidate `102bb13` launches with nxlink exit 0 at 14:42:15 CEST
(12:42:15.880070 UTC): 27,196,610 compressed bytes / 2,286 blocks.
The 74,436,664-byte NRO SHA-256 is
`883cee73794d4dd90853a37ba9a62f10a94eb16e1b1d181f384078a714fe1063`.
Its source/code checks and private gate are revalidated before transfer.
Full SD readback after this trial independently matches the candidate.
The user reports USB ready without a fresh screen observation.

All 39 reports / 841,828 bytes match independent second USB reads by size
and SHA, verified at 13:02:28.048108 UTC. Twenty-one change from the
preceding IA8/clamp trial and eighteen retain their previous bytes. Retained
files are not fresh evidence. FIFO capture and frame dumping have fresh
run IDs and remain disabled; no new image or replay is accepted.

The changed texture status records `load-pass`, object `0x80398EB8`, slot 0,
source `0x116E7E00`, 32 × 32 IA4, repeat S/T, no mipmaps, complete 1,024-byte
span. The checked production bridge writes this only after native
`GXLoadTexObj` returns and guest state is published. Execution then reaches
an independently recorded later Fog guard. This accepts the last retained
IA4/repeat load on this relocated source. The preceding exact source
`0x116E7DE0` is not separately retained returning, and IA4/clamp remains
synthetic coverage.

Discovery records one DrawQuad entry at dispatch 650547, LR `0x8007B294`,
stack `0x80398F58`, fiber `0x80347498`, position `0x80398F60`, size
`0x92133DF8`, one UV array at `0x92133EAC`, absent colors, alpha `0xFF`.
A distinct target `0x805E805C` carries its stage at dispatch 650816 on the
same fiber. The checked outer caller returns immediately after DrawQuad,
and the bridge cannot dispatch guest calls: one scoped quad return is
accepted through caller progression, not every layout. Later layout/text
calls reach `0x8007D520` and `0x805CF2BC`.

The terminal is `GX_SET_FOG_UNPROVEN_ARGS`, target `0x801722CC`, dispatch
652279, 52.465 seconds after the first translated dispatch. PC `0x800060A4`,
LR `0x805CF37C`, stack `0x80398F28`, type r3=0, color pointer r4=`0x80398F30`.
All four f64 argument representations are positive zero; the readable color
is transparent black. The checked caller loads one f32 value, assigns it to
f1..f4 `.d`, copies four color bytes to its stack and calls Fog with type 0.
The [bounded correction](GX_FOG_ZERO_2026-10-10.md) admits this third exact
Fog tuple while preserving the pinned native register work.

The changed graphics report records renderer readiness, FIFO write/work and
present milestones without a Dawn uncaptured error. No exception report
appears among the retrieved project files. The fresh sampled heartbeat
retains main reached and six TaskThread entries; it precedes the terminal
and does not prove later scheduler state. Five uncontrolled presentation
windows cover 102 frames / 45,589 ms, about 2.237 Hz weighted, ranging from
0.222 to 8.395 Hz. Dispatch/time totals vary between runs and do not establish
a performance improvement or regression. Fresh pixels and playability remain
unproven. Game data, generated callers, NROs and raw reports stay private.
