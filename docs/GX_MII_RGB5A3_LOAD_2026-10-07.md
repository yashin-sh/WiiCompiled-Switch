# Captured Mii RGB5A3 texture load — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_LOAD_FRONTIER.md)
accepts the second I4 load and stops at the third Mii object `0x80397D40`,
slot 0, RGB5A3 44×32. Admit only this identity and all eight captured words,
including full format 5 in both descriptor fields, physical MEM2 payload
`0x109C0C40`, clamp/clamp, no mipmap and the complete **2,816-byte** range.
Pinned native tiling is 4×4 texels / 32 bytes, yielding 11×8 tiles.

Reuse existing Aurora init, linear/linear zero-range LOD with edge disabled,
null user data and slot-0 binding before guest bookkeeping. This object retains
its own host GXTexObj and independent cache reuse. CPU, descriptor and payload
bytes remain unchanged. RGB565, IA8 and both captured I4 objects remain intact;
unknown identities/words/slots, missing/short ranges, null pointers and native
exceptions retain durable diagnosed stops. No dispatch trait, API or provider
is added, and no proprietary texture payload enters the public fixtures.

Independent synthetic fixtures encode five objects across four formats.
ASan/fatal UBSan/LSan contracts pass **18 valid loads per mode**, **1,348
headless / 1,358 rendered diagnosed refusals**. Checks cover every bit in
all eight descriptor words, mapped wrong identities, all unauthorized slots,
full/missing/short data and descriptor ranges, CPU/guest byte preservation,
separate/reused host objects and native-before-bookkeeping order. The native
seam also verifies that RGB5A3 is forwarded as format 5, not RGB565.

All twelve compiled mutations are rejected. All 22 local suites, rendered
SDK compilation, full synthetic retention and lint pass on code `e6a4727`.
The immutable-image, network-disabled private Rendered Discovery build passes;
71 required strong functions and 48 scoped unique providers are verified across
235 host inputs, 19 Rust archives and seven named libraries. The NRO contains
**73,621,560 bytes**, SHA-256
`4794cdc6ad5b3a203b6eb86a1d77c3a6c1433d7e5169f85c700b6e30ca5b9641`.
Dependency pins and existing upstream patch bytes/ns mtimes are preserved.
Final-head CI, merge and verified deployment evidence are tracked in
[PR #322](https://github.com/yashin-sh/WiiCompiled-Switch/pull/322). Deployment
requires all five workflows / six actual jobs at that final head; subsequent
Markdown updates must leave every non-Markdown built input unchanged. The [10:16 Netloader run](HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_LOAD_FRONTIER.md)
now accepts RGB5A3 and intervening helper return before a distinct I4 36×32
load. GPU completion and recognizable images still require fresh evidence. Private products, NROs and raw archives stay excluded.
