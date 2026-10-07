# Captured relocated source for the second Mii I4 object — 2026-10-07

The [fresh Switch run](HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_RELOCATED_DATA_FRONTIER.md) establishes first relocated-source load
and helper return, then stops at `0x80397DC0`, slot 0, I4 32×64,
physical data `0x109C1A20`, word 3 `0x0084E0D1`. Add only this second
captured identity to the existing relocated-source guard. All eight words,
both format fields, native dimensions, clamp/clamp, no mipmap and the full
**1,024-byte** tiled range remain mandatory. The next-pass object
`0x80397F80` retains only its earlier source `0x109C1A40`; unknown tuples stop.

Both admitted sources now have the captured first/second pair. Their native
objects remain independently cached by guest identity, including when either
object changes between the old and relocated source. Every load reinitializes
its own native object from the current checked source, then runs LOD/user-data/
binding before guest bookkeeping. CPU, descriptor and payload bytes stay
unchanged. No API, dispatch trait, provider or allocation policy is added.
Missing/short mappings, null native pointers and native exceptions remain
separately diagnosed stops.

Thirteen independent descriptor fixtures across eleven guest/native identities
and four formats exercise repeated first/second old → relocated → old
transitions and shared-source independence. Synthetic patterns contain no game
data. ASan/fatal UBSan/LSan contracts pass **40 loads per mode**,
**3,527 headless / 3,553 rendered refusals**. Every bit of all eight words,
wrong slots/identities, unknown next-pass relocated-tuple transplant,
missing/31-byte descriptors, missing/512/992/1023-byte data, native arguments
and CPU/guest preservation are covered.

Targeted contracts, SDK rendered branches, full synthetic retention, lint and
the immutable-image, network-disabled private Rendered Discovery build pass
on code `46f1d3b2c76325ecc37bd960544ca69c3d6569cd`. The ELF retains 71 required strong functions;
the scoped audit verifies 48 unique providers across 235 host inputs,
19 Rust archives and seven named libraries. NRO **73,621,560 bytes**,
SHA-256 `af9575be2b491e7f0bfe5b8e8f08581936ebd91be2a169017a38f86bdb4d872d`. Original pins and private upstream patch
bytes/ns mtimes stay preserved.

All 42 compiled mutations are rejected (compile exit 0; real SIGABRT -6),
and all 22 local suites pass on the exact non-Markdown built inputs. Five
final-head workflows / six actual jobs pass on `f8b82839441d6e7ffedf7b4ca40e5babdd6ff980`.
[PR #331](https://github.com/yashin-sh/WiiCompiled-Switch/pull/331) merges as
`265839ddb8901a595e43646042dffbc3fb28a865` at **18:02:36 CEST**
(16:02:36 UTC); the fetched main tree equals the validated candidate.
The exact validated NRO is ready locally. **SD copy and complete readback are
pending**, because the Switch is no longer detected in USB/MTP. No new copy
or execution is claimed. Issue #117 records the completed gates and pending
deployment. Subsequent Markdown-only
changes preserve every non-Markdown built input. Second relocated-source
return, next-pass return, GPU completion and game pixels require fresh
hardware evidence. Private products remain excluded.
