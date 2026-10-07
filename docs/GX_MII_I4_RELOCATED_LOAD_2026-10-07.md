# Captured relocated first Mii I4 source — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_I4_RELOCATED_DATA_FRONTIER.md) stops at the first Mii I4 load,
object `0x80397D80`, slot 0, 32×64, with physical data `0x109C1A20` and
word 3 `0x0084E0D1`. It has not reached the next-pass object `0x80397F80`.
The correction adds this exact source tuple **only for the first object**.
All eight descriptor words, both format fields, dimensions, clamp/clamp,
no mipmap and the full **1,024-byte** tiled range remain mandatory.
The earlier source `0x109C1A40` stays admitted for its three captured identities;
other objects do not inherit the newly captured source. Unknown tuples stop.

The existing native object cache remains keyed by guest identity. Every load
reinitializes the cached native object from the current checked source,
then performs LOD/user-data/binding before guest bookkeeping. Changing the
captured source keeps the same native object allocation and refreshes its data.
CPU, descriptor and payload bytes remain unchanged. No API, dispatch trait,
provider or guest address policy is added. Missing/short mappings, null native
pointers and native exceptions retain diagnosed stops.

Twelve independent descriptor fixtures cover eleven guest/native identities
and four formats, including old → relocated → old source transitions.
Synthetic patterns contain no game data. ASan/fatal UBSan/LSan contracts pass
**35 loads per mode**, **3,255 headless / 3,279 rendered refusals**. Every bit
of all eight words, wrong slots/identities, relocated-tuple transplants onto
the other two I4 objects, missing/31-byte descriptors, missing/512/992/1023-byte
data, native dimensions/format, shared-data independence and CPU/guest
preservation are checked.

Targeted contracts, rendered SDK compilation, full synthetic retention, lint
and the immutable-image, network-disabled private Rendered Discovery build
pass on code `1a8873c594e3169edc957fce319fce18b032846f`. The ELF retains 71 required strong functions;
the scoped audit verifies 48 unique providers across 235 host inputs,
19 Rust archives and seven named libraries. NRO size **73,621,560
bytes**, SHA-256 `a87188951902f5836086eb47e834a171b44b666e1bd2558b77effb83ba52a5f1`. Original pins and private upstream
patch bytes/ns mtimes stay preserved.

Forty compiled mutation checks and all 22 local suites are required before
deployment. Final-head CI, merge and complete SD readback are tracked with
the development PR and issue #117. Subsequent Markdown-only changes preserve
all non-Markdown built inputs. New-source return, next-pass return, GPU
completion and recognizable game pixels require a fresh hardware run.
Private products remain excluded.
