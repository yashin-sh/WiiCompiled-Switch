# Bounded TEV constant selectors — 2026-10-11

The [corrected RGB5A3 trial](GX_RGB5A3_REPEAT_2026-10-11.md) records the observed
32 × 32 repeat texture as `load-pass`, then pauses at DIRECT
`GXSetTevKColorSel` (`0x80171F30`). The durable frontier is dispatch 656,213,
stage 0, selector 31, after 41.207 seconds of translated execution. Selector 31
is the SDK's `GX_TEV_KCSEL_K3_A`. The color call is observed at entry; its native
return has not yet been established.

## Audited implementation scope

The correction adds separate checked bridges for color selection
(`0x80171F30`) and alpha selection (`0x80171F80`). Alpha selection is a conditional
pre-port from the audited scalar family, not a call witnessed or returned in the
current console result.

Pinned WiiCompiled `a135beb201042b20f390c6695ca6b26768820fb4` forwards the stage
and selector unchanged to native GX. Both bridges admit only stages 0–15 and
defined SDK selectors:

| Bridge | Admitted selectors | Reserved holes rejected |
| --- | --- | --- |
| K color | 0–7 and 12–31 | 8–11 |
| K alpha | 0–7 and 16–31 | 8–15 |

Values outside these domains stop before native output. The bridges preserve
the complete guest CPU context, forward legal values without truncating them,
and expose distinct diagnostic stages and dispatch traits.

The pinned writer changes a five-bit selector field in the shared stage-pair BP
register. Color/alpha fields use different shifts for even and odd stages. The
paired stage, the other selector, and shared swap-table channels must remain
unchanged. The pinned Aurora decoder consumes that BP packet and marks the
pipeline dirty. The correction uses the existing native writer/decoder path;
it does not fabricate guest texture or material contents.

## Validation and remaining gates

The production trait/bridge contracts pass 4,992 admitted cases and 58 diagnosed
refusals in each headless/rendered mode under ASan, fatal UBSan and LeakSanitizer.
They extract the pinned native writer and BP decoder as oracles and independently
check register masks, packet bytes, dirty flags, paired/shared-field preservation,
unchanged CPU state, invalid stages, reserved selectors and high-bit inputs.

These local contracts establish bounded argument/native-packet behavior. The
new KSEL candidate's rendered/Discovery syntax gate, separate exact-source
private rendered build, complete exact-HEAD published checks, verified SD
deployment and corrected console execution remain pending. Success of the
preceding RGB5A3 candidate's gates does not validate this new code revision.

Keep automatic restart acceptance separate from manual launch evidence. A later
durable frontier or attributable milestone is still required to accept the
observed K color return. Any future alpha return needs its own attributable
console evidence. No new game pixels or playability are claimed.
