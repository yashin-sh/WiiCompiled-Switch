# Bounded TEV constant selectors — 2026-10-11

The [corrected RGB5A3 trial](GX_RGB5A3_REPEAT_2026-10-11.md) records the observed
32 × 32 repeat texture as `load-pass`, then pauses at DIRECT
`GXSetTevKColorSel` (`0x80171F30`). The durable frontier is dispatch 656,213,
stage 0, selector 31, after 41.207 seconds of translated execution. Selector 31
is the SDK's `GX_TEV_KCSEL_K3_A`. That entry was the preceding frontier; the
corrected candidate now advances beyond that frontier to the indirect setter.

## Corrected console result

Candidate `4e5087d` passes complete SD size/SHA-256 readback at the same project
destination, with verified local backup and removal of older project candidates.
The replacement is 73,711,672 bytes, SHA-256 identity `22dac60ee6a2eda3…`.
Fresh reports collected after deployment record DIRECT `GXSetTevIndirect`
(`0x801717AC`) at dispatch 659,106, after 49.175 seconds of translated execution.
The preceding diagnostic stage is `RMCP01_GX_SET_TEV_K_ALPHA_SEL`. This stage
and the distinct later frontier support selector progression. In the audited
material caller, K color selection, alpha input/operation, K alpha selection and
the indirect setter form one ordered block without an intervening branch.
The report does not record dynamic caller identity or individual return events;
the source association is an inference, and LR alone is not exclusive proof.
Alpha selection is now witnessed on the console path.

This evidence establishes corrected NRO execution and the later blocked frontier.
It does not establish how the NRO was relaunched: no new SDK restart command was
sent during collection, and an operator-confirmed manual launch is not part of
this receipt. The live subscription began after execution had paused and received
only 63 bytes; it does not provide a complete live history of the selector calls.
The fresh durable report supplies the frontier and preceding-stage evidence. No new frame-rate,
menu-pixel or playability result is accepted.

## Audited implementation scope

The correction adds separate checked bridges for color selection
(`0x80171F30`) and alpha selection (`0x80171F80`). The corrected console path
reaches the later indirect setter after the alpha-selection diagnostic stage.
This does not establish every admissible stage or selector on hardware.

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
KSEL code candidate `4e5087d` passes its own rendered HLE/input/Discovery/capture
SDK syntax gate, strict debug-server compile and separate exact-source private
rendered build. All eight published checks on that exact HEAD are successful.
These results belong to the KSEL revision, independently of the preceding
RGB5A3 candidate's validation.

The private provider audit checks 100 unique scoped providers across 281 explicit
inputs. The final link retains 114 strong functions, including both new bridges
and native KSEL functions, plus three GX state objects. The source snapshot
matches 866 public inputs; the original nine private patch files retain their
bytes and nanosecond modification times. Provider uniqueness remains scoped to
the explicitly audited inputs.

The exact candidate NRO is 73,711,672 bytes, SHA-256 identity `22dac60ee6a2eda3…`;
its full hash and build receipts stay private. Discovery, FIFO capture and frame
dump are disabled in this built NRO, matching the preceding trial. Verified SD
deployment and the corrected console progression described above pass.

Automatic NRO restart remains unvalidated. The next [bounded TEV indirect
correction](GX_TEV_INDIRECT_2026-10-11.md) has separate build, CI, deployment and
console gates; the successful KSEL gates do not validate that new candidate.
