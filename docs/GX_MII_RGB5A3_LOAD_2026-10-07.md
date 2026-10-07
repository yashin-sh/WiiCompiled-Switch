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

Twelve compiled mutations, all 22 local suites, rendered SDK compilation,
full synthetic/private rendered builds and exact-head CI are in progress.
Deployment requires every local gate and all five final-head workflows / six
actual jobs. RGB5A3 return, GPU completion and recognizable images require
fresh hardware evidence. Private products, NROs and raw archives stay excluded.
