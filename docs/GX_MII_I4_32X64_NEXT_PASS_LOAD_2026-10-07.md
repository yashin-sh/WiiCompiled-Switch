# Captured I4 object in the next Mii pass — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_I4_32X64_NEXT_PASS_FRONTIER.md) accepts I4 16×16 and completion of the
previous Mii caller pass, then stops at object `0x80397F80`, slot 0,
I4 32×64, LR `0x800C444C`. The correction adds only this observed identity
to the existing exact 32×64 descriptor guard. All eight words, both format
fields, physical data `0x109C1A40`, dimensions, clamp/clamp and no mipmap
remain mandatory. The complete tiled range is **1,024 bytes**.

The three 32×64 objects share data, but each native GXTexObj remains cached
by its own guest identity. Init/LOD/user-data/binding precede guest bookkeeping.
CPU, descriptor and payload bytes remain unchanged. No API, dispatch trait
or provider is added. Unknown identities/tuples, short mappings, null native
pointers and native exceptions retain diagnosed stops.

Eleven independent objects across four formats test every descriptor bit,
wrong mapped identities/slots, full and missing/short descriptor/data ranges,
native dimensions/format, distinct reuse and CPU/guest preservation. Synthetic
payloads contain no game data. ASan/fatal UBSan/LSan contracts pass
**30 valid loads per mode**, **2,980 headless / 3,002 rendered refusals**.
The shared 32×64 triple and two other shared-data pairs remain independent.

Targeted contracts, rendered SDK compilation, full synthetic retention, lint
and the immutable-image, network-disabled private Rendered Discovery build
pass on code `551cdbf`. The native ELF retains 71 required strong
functions. The scoped audit verifies 48 unique providers across 235 host
inputs, 19 Rust archives and seven named libraries. NRO size is
**73,621,560 bytes**, SHA-256
`0b30ee1789163164671f6cb37e7a150dc6e3851e8c7875bdc99fb8cbe7437b24`. Pins and upstream patch bytes/ns mtimes stay preserved.

Thirty-five compiled mutation checks cover widened identity ranges/slots,
omitted words, short tiled ranges, wrong native dimensions/format, collapsed
host identities and CPU/bookkeeping changes. Their results, the 22-suite gate,
five final-head workflows / six actual jobs, merge and complete SD readback
are recorded with the development PR and
[issue #117](https://github.com/yashin-sh/WiiCompiled-Switch/issues/117).
Deployment requires every gate. Subsequent Markdown-only changes must preserve
every non-Markdown built input. New-object return, GPU completion and game
pixels require fresh hardware evidence. Private products remain excluded.

The [subsequent console run](HARDWARE_RESULTS_2026-10-07_MII_I4_RELOCATED_DATA_FRONTIER.md) is attributable to this exact NRO,
with a successful transfer at 16:48:04 CEST and verified 37-report retrieval.
It stops at the first Mii object `0x80397D80` using the separately captured
source `0x109C1A20`, before reaching this next-pass object. This candidate's
next-pass return remains unconfirmed; its validated guard is preserved.
