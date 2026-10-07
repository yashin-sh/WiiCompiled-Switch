# Second captured I4 36×32 object — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_SECOND_LOAD_FRONTIER.md)
accepts object `0x80397CC0` and stops at its sibling `0x80397D00`, slot 0.
Its complete descriptor and payload are identical. Extend only the exact
identity guard to these **two observed addresses**, retaining all eight words,
slot 0, I4 36×32, clamp/clamp, no mipmap and the full **640-byte** physical
MEM2 range at `0x109C1780`. Logical native width remains 36.

The pair shares one payload but keeps distinct, independently reused host
GXTexObj instances. Existing init/LOD/user-data/binding precede guest
bookkeeping; CPU, descriptor and payload bytes remain unchanged. Existing
RGB565, IA8, RGB5A3 and both 32×64 I4 objects remain intact. Unknown identities,
words and slots, short/missing ranges, null pointers and native exceptions
keep diagnosed stops. No new API, dispatch trait or provider is added.

Independent fixtures encode seven objects across four formats, with two
separate shared-data I4 pairs. ASan/fatal UBSan/LSan contracts pass **22 valid
loads per mode**, **1,892 headless / 1,906 rendered diagnosed refusals**.
Checks cover every bit of every word, wrong mapped identities, unauthorized
slots, full/missing/short descriptor/data ranges including 576/639-byte data,
complete CPU/guest preservation, seven distinct/reused host objects and
native-before-bookkeeping order. Native seams require width 36 and full
640-byte resolution for both new objects. No proprietary payload is embedded.

Nineteen compiled mutations, all 22 local suites, rendered SDK compilation,
full synthetic/private rendered builds and exact-head CI are in progress.
Deployment requires every local gate and all five final-head workflows / six
actual jobs. Second-object return, GPU completion and recognizable images
require fresh hardware evidence. Private products, NROs and raw archives stay
excluded.
