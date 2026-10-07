# Second captured RGB5A3 38×32 object — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_38X32_SECOND_LOAD_FRONTIER.md)
accepts object `0x80397C40` and its draw helper, then stops at sibling
`0x80397C80`, slot 0. The descriptors share all eight words and physical
MEM2 payload `0x109C0200`. Extend only the exact identity guard to these two
observed addresses, retaining slot 0, RGB5A3 38×32, all words, clamp/clamp,
no mipmap, zero/linear LOD and the full **2,560-byte** tiled range.
Native logical width stays 38.

The pair shares data but keeps distinct independently reused native GXTexObj
instances. Native init/LOD/user-data/binding precede guest bookkeeping;
CPU, descriptor and payload bytes are preserved. Existing RGB565, IA8, 44×32
RGB5A3 and both I4 pairs remain covered. Unknown identities, words and slots,
short/missing ranges, null pointers and native exceptions retain diagnosed
stops. No API, dispatch trait or provider is added.

Nine independent objects across four formats include three shared-data pairs.
Contracts cover every descriptor bit, wrong mapped identities/slots, full and
missing/short descriptor/data ranges including 2,432/2,559-byte data, distinct
host reuse, native width/format/order/failure and complete CPU/guest preservation.
ASan/fatal UBSan/LSan contracts pass **26 valid loads per mode**,
**2,436 headless / 2,454 rendered diagnosed refusals**. Synthetic fixtures contain no game data.

All twenty-seven compiled mutations are rejected. Targeted contracts, rendered
SDK compilation, full synthetic retention, lint and the immutable-image,
network-disabled private Rendered Discovery build pass on code `63df50a`.
The native ELF retains 71 required strong functions. Provider audits verify
48 scoped unique providers across 235 host inputs, 19 Rust archives and seven
named libraries. The private NRO contains **73,621,560 bytes**, SHA-256
`ccbf256986521c3d31e0634994eb56e87ff1bb34f858cb9307ddf9d40f96ae6a`.
Original dependency pins and upstream patch bytes/ns mtimes are preserved.

The 22-suite local gate, five final-head workflows / six actual jobs, merge
and full SD readback evidence are tracked in
[PR #327](https://github.com/yashin-sh/WiiCompiled-Switch/pull/327).
Deployment requires all of those gates; subsequent Markdown changes must
leave every non-Markdown built input unchanged. The second-object return is accepted by the subsequent checked run below;
GPU completion and recognizable images still require separate evidence. Private products, NROs and raw archives stay excluded.

The [15:39 CEST console run](HARDWARE_RESULTS_2026-10-07_MII_I4_16X16_LOAD_FRONTIER.md)
now accepts second-object, draw-helper and intervening-call return before a
distinct I4 16×16 stop. All 37 reports / 632,976 bytes are verified, thirteen
changed / twenty-four retained. Final visible outcome remains unconfirmed.
