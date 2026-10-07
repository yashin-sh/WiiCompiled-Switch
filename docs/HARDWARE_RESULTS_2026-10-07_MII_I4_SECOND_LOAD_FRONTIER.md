# First Mii I4 load returned; second object frontier — 2026-10-07

The [first I4 candidate](GX_MII_I4_LOAD_2026-10-07.md) returns through the
captured load at object `0x80397D80`, slot 0, then through the intervening Mii
draw helper. The next guarded load is at **object `0x80397DC0`**, with the same
complete eight-word descriptor and payload. The target remains GXLoadTexObj
`0x80170F2C`, reason `GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, dispatch
**631675 / 120.247 seconds**, LR **`0x800C4474`**, stack **`0x80397AE8`**.
The user confirms **black screen followed by an error**. No recognizable game
image, uploaded pixels or GPU completion is established.

## Exact run and attribution

Validated code is `076dce6ce323aee6fec1c2641031d644022aec68`. All 22 local
suites, eight-word/bit/refusal contracts, six rejected mutants, both SDK modes,
full synthetic/private rendered builds and five final-head workflows / six
jobs pass. PR #320 merges as `750ac3e`. Launch revision
`34d07028fcc31a12454281a4d1ffc7d838c43cde` differs only in Markdown.

The NRO is **73,621,560 bytes**, SHA-256
`e7cc019de0eb0c68c271a21135732abbee403797580a5593310468df658d53c7`.
Direct Netloader starts at **08:47:21 UTC** and exits **0 at 08:47:45 UTC**,
sending **26,798,123 compressed bytes / 2,253 blocks (36.40%)**. Candidate
hashes, dependency pins and the original upstream patch are checked before
transfer. The same SD file was independently verified by complete readback.

USB/MTP retrieval at **08:50:33 UTC** obtains **37 reports / 632,060 bytes**,
**13 changed / 24 identical**, versus the [viewport baseline](HARDWARE_RESULTS_2026-10-07_VIEWPORT_MII_I4_LOAD_FRONTIER.md).
Every size/hash, baseline difference, ZIP member byte and CRC is verified again.
Source timestamps, runtime build ID, guest bookkeeping words, native BP bytes
and pixels are unavailable. Private NROs, game products and raw archives stay
excluded from GitHub.

## Return evidence

The checked caller loads its first object at LR `0x800C444C`, then calls draw
helper `0x800C4B70`, then loads the second object at LR `0x800C4474`.
The fresh blocker identifies that distinct second call and restored caller
stack, with coherent fiber `0x80347498`. This sequence establishes return from
the admitted first native load and intervening helper. The load status file
is overwritten by the second refusal; no separate retained first-load pass
or individual BP command bytes are claimed.

The getter/depth path remains crossed: getter at dispatch 631496, setup helper
631497 and depth at 631518, before the new texture stop. The preceding heartbeat
at 631410 retains **5,988 FIFO writes, 102 successful presents / zero failures**,
six TaskThread hits, valid FST and coherent scheduler identities. The copy report
is byte-identical to the baseline and may be retained. Counters and elapsed
values do not establish pixels or a performance comparison.

Both captured objects have words
`00000190 00000000 0000FC1F 0084E0D2 00000000 00000000 00000000 00200102`:
I4, 32×64, clamp/clamp, no mipmap, physical MEM2 data `0x109C1A40`, 1,024 bytes.
The [second-object correction](GX_MII_I4_SECOND_LOAD_2026-10-07.md) admits these
two object identities, preserving every remaining guard and distinct host
objects for their shared payload. The second return remains unaccepted.
