# Hardware result — IA8 map 0 returned / map 1 frontier (2026-10-02)

Code candidate `1b5e4decbe730159e122e7316747c090fb62f792` produced the
73,289,784-byte rendered Discovery NRO with SHA-256
`25fa510ec56360bd5fa6b6fb0dd941110bb8d5773ffa049b0ddb2ef2c01b3d26`.
After an interrupted transfer, the retry completed with nxlink exit code 0
at 15:56:22 UTC (17:56:22 Europe/Paris). USB/MTP retrieval preserved 28 text
reports totaling 525,900 bytes.

The new refusal remains GXLoadTexObj `0x80170F2C`, kind
`GX_LOAD_TEX_OBJ_UNPROVEN_DESCRIPTOR`, stage `RMCP01_GX_LOAD_TEX_OBJ`.
It uses the same object `0x80384500`, eight descriptor words and 32-byte IA8
backing as the prior frontier, but r4 is now 1 and LR is `0x8024126C`, rather
than r4=0 / LR=`0x80241260`. The diagnostic size is now zero for an unapproved
call, confirming the candidate's changed refusal path.

The local translated caller `0x80241240` invokes the same object on maps
0 through 7 in order. Its map-0 call returns to `0x80241260`; only then can
the map-1 call with return address `0x8024126C` execute. Thus the observed
IA8 map-0 load returned on hardware. This is evidence for that particular
load, not acceptance of every GXLoadTexObj descriptor or every map. The
later refusal overwrites the same status file, so no separate map-0 load-pass
file survives. The caller audit remains local; game-derived code/payload is
not committed as a fixture.

Pinned WiiCompiled guards an eight-entry binding array before native loading.
Aurora uses eight-entry GXTexMode/Image register tables and IDs 0..7. The
bounded follow-up covers those legal scalar IDs only for the already captured
IA8 descriptor, forwards the actual ID, and preserves backing checks, sampler
state and guest mirror updates. IDs 8, 0xFF and 0xFFFFFFFF remain refused;
the old RGB565 descriptor retains its existing map-0 contract. Maps 1..7
remain hardware-unvalidated until a later caller or milestone is reached.

The changed post-main snapshot precedes these loads: main reached, six
TaskThread::run hits, coherent guest fiber / OS current / running at
`0x80347498`, valid 64,224-byte FST at `0x97DC0000`, initialized renderer
and active frame. It retains 1,556 FIFO writes, 99 GXCopyDisp calls,
99 successful presents and zero present failures. It cannot independently
prove IA8 bind writes or visual game pixels. No exception report was retrieved.
Changed resource reports retain 19 successful DVD reads and SZS decode-pass
at 499,251 consumed / 3,153,052 produced bytes.

MTP timestamps remain unusable. Thirteen reports differ from the prior run;
the new map ID, later return address, changed refusal-size semantics and
audited call order establish candidate progression. Identical ancillary
reports are not independently dated. Static coverage remains 141 native,
10,494 translated and 314 missing among 10,949 direct targets, with 835
runtime-seen / 19 runtime-seen missing. Raw reports, hashes and bundle stay
local. Visual correctness is still unverified.
