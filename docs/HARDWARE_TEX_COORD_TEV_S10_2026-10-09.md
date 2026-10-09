# Observed texture-matrix return; signed TEV color stop — 2026-10-09

The real-Switch trial of `662c388` accepts the observed matrix-30 Gen2 return
through checked caller progression. It reaches `GXSetTevColorS10`, PAL
`0x80171E70`, at dispatch 651187 / 53.437 seconds. The terminal arguments are
register ID 1 and color pointer `0x80398E80`, with LR `0x800815F8` and the
texture-load stage. This direct boundary has arrived, not returned.

## Scoped progression proof

The former refusal was `GXSetTexCoordGen2(0,1,4,30,0,125)`, with LR
`0x800814D8` in caller `0x80081210`. The checked caller performs that Gen2
loop before its conditional matrix helper. The fresh trace reaches helper
`0x8007F310` at dispatch 651168 / LR `0x80081538`, with the Gen2 stage and
retained matrix word 30. It subsequently reaches texture preparation at
dispatch 651172 with the immediate-matrix stage, then the distinct S10 stop
on the same caller stack. These witnesses, the caller control flow and the
candidate's guarded native forwarding support the matrix-30 return.

This is a control-flow inference, not an individually captured Gen2 return
instruction. The early Gen2 first-hit line still describes an identity call;
it must not be relabeled as the later matrix-30 entry. Other matrices and
coordinates remain outside the new admission, and transform pixels are unproven.

The fresh graphics report contains neither the previous missing-`CopySrc`
validation error nor its invalid-command-buffer error. This establishes that
those logged failures did not recur on this trial, not correctness of every
copy or new visible pixels. Both capture controllers have fresh run identifiers
and remain `DISABLED`; retained images and replay files are not attributed.
The operator confirmed USB readiness without another screen observation.

## Candidate and verification

[PR #349](https://github.com/yashin-sh/WiiCompiled-Switch/pull/349) merged after
all eight published exact-HEAD checks succeeded. The same candidate passed
the separate private rendered build, SDK syntax, scoped unique-provider and
retention audits, Gen2 contracts and the complete desktop GPU suite.

The exact NRO is 74,432,568 bytes, SHA-256
`111019c533a66756fee86d3de358c3c390efc7e0b0b695ca9e6aaa0cb95f7068`.
Netloader confirmed its transfer with exit 0 at 21:01:24 UTC. Complete SD
readback matches the artifact again. All 39 reports / 837,688 bytes also pass
a second independent USB copy and hash comparison: nineteen differ from the
preceding depth-texture baseline and twenty are retained. MTP timestamps are
unavailable; retained files are not fresh evidence. No exception report appears.

The sampled report at dispatch 648010 precedes the terminal boundary. It has
coherent guest-fiber / OS current / OS running identities, a valid FST and
six TaskThread entries. It records 6,054 FIFO writes, drawable work, 103
display copies and 103 successful presents, with zero recorded presentation
failures. These counters do not establish later GPU completion or pixel contents.
Five presentation windows total 102 frames over 47.377 seconds, about 2.153 Hz,
with individual rates of 0.215–9.132 Hz. Loading and stalls are included; this
is not steady gameplay FPS or a controlled performance comparison.

## Next attributed boundary

The pinned wrapper identifies `0x80171E70` as `GXSetTevColorS10`. Its color
input is eight bytes: four big-endian signed 16-bit RGBA components. The native
writer keeps each component's low eleven bits and emits the RA/BG BP register
pair for the selected TEV register. The current blocker records only the
pointer, not those eight bytes or their readability. Actual component values
remain unknown; no guest color payload is published or invented here.

Raw reports, captures, game-derived translations and private NROs remain local.
