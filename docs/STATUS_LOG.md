# Status log

Archived from the project README on 2026-10-07. This preserves the dated evidence and candidate/deployment history; use [the roadmap](../ROADMAP.md) for the current frontier.

2026-10-09: [signed TEV color bridge](GX_TEV_COLOR_S10_2026-10-09.md)
adds the missing direct target through the source-owned extension registry.
All four IDs and complete eight-byte guest inputs are checked before native
output. ASan/UBSan passes 196,610 pinned writer/decoder packets and both-mode
refusals. Candidate `d5f24b9` passes the private rendered build, 70-file SDK
syntax, 82 scoped providers / 100 retained functions and all eight published
exact-HEAD checks. PR #350 merges as `25a95c4`; full SD readback verifies the
74,432,568-byte NRO at 21:27:45 UTC. Fresh console execution remains pending;
the observed color payload is still unknown.

2026-10-09: [matrix-30 console result](HARDWARE_TEX_COORD_TEV_S10_2026-10-09.md)
accepts Gen2 return through checked caller progression and reaches
`GXSetTevColorS10`, ID 1 / pointer `0x80398E80`, at 53.437 seconds. All 39
reports / 837,688 bytes pass independent USB verification; nineteen change
and twenty are retained. The preceding Dawn validation errors are absent
from the new graphics report. Captures remain disabled; actual S10 components
and new screen pixels are not recorded.

2026-10-09: [matrix-30 and snapshot usage candidate](GX_TEX_COORD_SNAPSHOT_2026-10-09.md)
forwards the exact newly observed Gen2 matrix and adds `CopySrc` to the
checked snapshot descriptor. Both-mode guards and 36 actual pinned native
packets pass. The new exact RGBA8 GPU test fails with the old descriptor
and passes after correction; the full existing desktop suite also passes.
Private-build, complete CI and console gates remain pending.

2026-10-09: [depth-texture console result](HARDWARE_Z_TEXTURE_TEX_COORD_2026-10-09.md)
accepts the observed disable/Z8/zero-bias return through later dispatches.
All 39 reports / 837,530 bytes pass independent USB verification; eighteen
change and twenty-one are retained. The terminal guard is texture-coordinate
generation with matrix 30. Dawn separately rejects a texture-copy source
without `CopySrc` and its command buffer. Captures remain disabled; no new
screen or pixel observation is attributed.

2026-10-09: [normal-matrix/depth-texture trial](HARDWARE_NORMAL_Z_TEXTURE_2026-10-09.md)
records the normal-matrix helper and later dispatches before a `GXSetZTexture`
diagnostic stop. Captures remain disabled; no new screen observation or pixel
capture is attributed. The new bridge covers the native depth operation/format/
bias encoding, with executable writer and BP decoder contracts. Private build
and complete CI are required before merging; hardware return remains pending.

2026-10-09: [light/normal-matrix trial](HARDWARE_LIGHT_NORMAL_MATRIX_2026-10-09.md)
records light entry followed by later dispatches, then deliberately aborts at
`GXLoadNrmMtxImm`. The operator sees the Wiimote warning page and a crash.
Captures remain disabled. The new bridge converts a complete guest matrix for
Aurora's normal-matrix FIFO path; sanitizer packet contracts pass, with private
build/complete-CI gates required before merge and hardware return still pending.

2026-10-09: [merge-policy correction](CI_MERGE_POLICY_2026-10-09.md) restores
all visible checks on PRs #339, #341, #344 and #345 through eighteen reruns.
The new gate checks the complete exact-HEAD rollup; main requires all eight
Actions checks and enforces protection for administrators. Competing development
push/PR runs are removed without removing validation suites.

2026-10-09: [scissor/light trial](HARDWARE_SCISSOR_LIGHT_2026-10-09.md) accepts
the observed scissor helper returning, then stops at native light loading. Captures
remain disabled; no new pixels are attributed. The candidate converts the full
light object and covers all eight IDs, with actual pinned FIFO contracts and a
source-owned missing-direct-call registry. Private/CI/hardware gates are pending.

2026-10-09: [projection/scissor-origin run](HARDWARE_PROJECTION_SCISSOR_2026-10-09.md)
accepts projection-getter return through checked caller progression, then stops
at GXSetScissorBoxOffset(0,0). Captures are disabled; no new pixels or visual
observation are attributed. The existing viewport suite now validates the
representable scissor-origin family and pinned BP output. Remaining build and
hardware gates are pending.

2026-10-09: [capture-disabled KD/projection trial](HARDWARE_KD_PROJECTION_2026-10-09.md)
accepts the post-resume request and close on handle 2004, then diagnoses
GXGetProjectionv at about 61 seconds. No new pixels are attributed with captures
disabled. Shared matrix/vector projection state and the save/restore pair now
pass local rendered/headless contracts; remaining gates and hardware are pending.

2026-10-09: [recognizable Switch image and desktop replay](HARDWARE_NONBLACK_REPLAY_2026-10-09.md)
retain the Wiimote safety page at frame 4 in both selected-copy and final-surface
PNGs. Desktop replay reproduces that page and validates the complete saved
90-frame prefix; frame 90 remains black. The later KD post-resume request is
not reached. The next run disables captures on the same validated candidate.
Three additional old project copies/transfer aliases were verified, archived and
removed, leaving the current candidate.

2026-10-09: [sampled diagnostic SD writes trial](HARDWARE_KD_POST_RESUME_2026-10-09.md)
records early 7.5–9.3 Hz, later stalls and a 2.968 Hz window average with captures
disabled. A fresh terminal report stops at the fifth KD request, command 2 after
resume. The bounded live-handle and scheduler-phase correction awaits console
acceptance; no new pixel or visual observation is attributed to this run.

2026-10-09: [capture-disabled startup measurement](HARDWARE_PRESENT_RATE_2026-10-09.md)
confirms both capture controllers were disabled. Eight present windows average
about 1.18 Hz; the operator sees the Wiimote warning page, then black. No terminal
blocker is recorded. Recurring diagnostic SD writes are now sampled, with hardware
impact pending. Forty old project NROs were backed up, verified and removed.

2026-10-09: [texture-family/selected-XFB console trial](HARDWARE_CAPTURE_CONTROL_2026-10-09.md)
advances beyond the former Mii load guard. The operator reports visible boot images
and low FPS. Saved frame-1/90 pixels remain black; manual exit precedes a final
frame-102 checkpoint. FIFO recording exhausts its budget after four frames, with
only the first prefix saved. Capture/control corrections and a measured trial
are pending.

2026-10-08: [corrected Switch first/latest image result](HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md)
validates SD replacement and the completed-frame checkpoint. First and last GPU
images contain black RGB, with alpha 255 and 0 respectively; game rendering remains
unproven. The texture refusal is unchanged from the preceding surface trial.

2026-10-08: [first actual Switch GPU image](HARDWARE_SURFACE_FIRST_IMAGE_2026-10-08.md)
decoded as opaque black. The latest-image replacement failed on SD at frame 30;
the capture is partial, and the correction requires a console retest. The same run
advances past the previous next-pass I4 load/helper to the next guarded identity.

Experimental Nintendo Switch (Horizon OS / Atmosphère) homebrew porting layer for [WiiCompiled](https://github.com/patchzyy/Wiicompiled).

## Goal

Run a legally-owned Mario Kart Wii dump through the WiiCompiled static-recompilation runtime as a native AArch64 Nintendo Switch homebrew application (`.nro`), without Dolphin at runtime.

## Project progress

**First real RMCP01 FIFO/Aurora render work: ✅ hardware validated**

**First game-facing RMCP01 GPU present: ✅ hardware validated**

**Visually confirmed Mario Kart Wii image: ❌ not yet proven**

The project now executes real WiiCompiled-translated RMCP01 code on Switch,
loads real user-owned boot/StaticR/Home Button resources, produces real GX FIFO
work, and presents frames successfully through Aurora → Dawn/WebGPU →
Vulkan/NVK. The next work is to advance game/UI initialization, verify GX
state and resources on the executed path, and establish recognizable Mario
Kart Wii pixels. Present counters alone do not identify why the observed
screen remains black.

## Current status

The current engineering review and remaining CI/runtime risks are recorded in
[`docs/PORT_AUDIT_2026-10-03.md`](../docs/PORT_AUDIT_2026-10-03.md).

The post-`main` fast-track tracked in issue #117 has hardware-crossed the
scheduler/resource/render path through real FST/DVD/SZS/StaticR loading,
TaskThread execution, real FIFO work, `GXCopyDisp`, repeated successful
presents, and multiple Home Button/UI texture-object setup calls.

The latest [Mii console run](../docs/HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_RELOCATED_DATA_FRONTIER.md) establishes the first
relocated-source native I4 load and intervening draw-helper return through
checked unconditional caller inference. It next stops at **GXLoadTexObj
(`0x80170F2C`)**, object **`0x80397DC0`**, slot 0, I4 **32×64**,
data **`0x109C1A20`**, at dispatch 633149 / 122.796 seconds. All 37 reports /
632,567 bytes are verified, seven changed / thirty retained. The exact PR #330
NRO transfers successfully at 17:30:50 CEST. Retained copy/display-list reports
are not fresh GPU or pixel evidence. Second relocated-source and next-pass
return, GPU completion and game pixels remain unconfirmed.

The [second relocated-source correction](../docs/GX_MII_I4_SECOND_RELOCATED_LOAD_2026-10-07.md) adds only that
observed identity to the existing exact eight-word source guard and complete
1,024-byte range. Both source pairs retain independent native objects and
refresh the current source on each load. The next-pass identity retains only
its earlier captured source. Thirteen descriptors / eleven identities pass
40 loads per mode and 3,527 / 3,553 refusals. SDK/synthetic/private builds and
lint pass, retaining 71 strong functions and 48 scoped providers. NRO SHA-256
`af9575be...`, size 73,621,560 bytes. All 42 mutation checks and 22 local suites pass. Five final-head workflows /
six jobs pass on `f8b8283`; [PR #331](https://github.com/yashin-sh/WiiCompiled-Switch/pull/331) merges as `265839d`. The exact NRO is ready locally; SD copy and complete
readback await the Switch's USB/MTP connection. Second relocated-source and
next-pass return require a fresh console run.

The [first relocated-source correction](../docs/GX_MII_I4_RELOCATED_LOAD_2026-10-07.md)
is merged and hardware-crossed for its first captured load/helper path. The
[earlier next-pass correction](../docs/GX_MII_I4_32X64_NEXT_PASS_LOAD_2026-10-07.md)
is merged and locally/CI validated; this run has not reached its next-pass
object. Earlier accepted returns remain scoped to their captured source tuples.

The [second-38×32 correction](../docs/GX_MII_RGB5A3_38X32_SECOND_LOAD_2026-10-07.md)
passes 22 suites, 27 mutations, SDK/synthetic/private builds and five final-head
workflows / six jobs. [PR #327](https://github.com/yashin-sh/WiiCompiled-Switch/pull/327)
is merged as `0d4fff0`; its 73,621,560-byte NRO, SHA-256 `ccbf2569...`, has
verified SD readback and now returns through the second observed load.

The [first-38×32 correction](../docs/GX_MII_RGB5A3_38X32_LOAD_2026-10-07.md)
passes 22 suites, 25 mutations, SDK/synthetic/private builds and five final-head
workflows / six jobs. [PR #326](https://github.com/yashin-sh/WiiCompiled-Switch/pull/326)
is merged as `101fee4`; its 73,621,560-byte NRO, SHA-256 `4fd7e46e...`, has
verified SD readback and now returns through the first observed load.

The [second-36×32 correction](../docs/GX_MII_I4_36X32_SECOND_LOAD_2026-10-07.md)
passes all 22 suites, nineteen mutation checks, SDK/synthetic/private builds,
and five final-head workflows / six jobs. [PR #324](https://github.com/yashin-sh/WiiCompiled-Switch/pull/324)
is merged as `1df611f`; its 73,621,560-byte NRO, SHA-256 `bf093aea...`, has
verified SD readback and now returns through the observed second load.

The [first 36×32 correction](../docs/GX_MII_I4_36X32_LOAD_2026-10-07.md) passes
22 suites, seventeen mutants, SDK/synthetic/private builds and five final-head
workflows / six jobs. [PR #323](https://github.com/yashin-sh/WiiCompiled-Switch/pull/323)
is merged as `c46f2ad`; its verified 73,621,560-byte NRO, SHA-256
`e2ae9043...`, transfers with nxlink exit 0 at 13:32:04 CEST and now returns
through its observed load.

The [RGB5A3 correction](../docs/GX_MII_RGB5A3_LOAD_2026-10-07.md) passes 22 suites,
twelve mutants, SDK/synthetic/private builds and five final-head workflows /
six jobs. [PR #322](https://github.com/yashin-sh/WiiCompiled-Switch/pull/322) is
merged as `8938122`; its verified 73,621,560-byte NRO, SHA-256 `4794cdc6...`,
transfers with nxlink exit 0 at 10:17:06 UTC and now returns through its load.

The [second I4 correction](../docs/GX_MII_I4_SECOND_LOAD_2026-10-07.md) passes
22 local suites, eight mutants, SDK/synthetic/private builds and all five
final-head workflows / six jobs. [PR #321](https://github.com/yashin-sh/WiiCompiled-Switch/pull/321)
is merged as `4da5100`; its verified 73,621,560-byte NRO, SHA-256
`fe2a28d9...`, transfers with nxlink exit 0 at 09:42:22 UTC and now returns
through the second observed load.

The [first I4 candidate](../docs/GX_MII_I4_LOAD_2026-10-07.md) passes 22 suites,
six mutants, SDK/synthetic/private builds and five final-head workflows / six
jobs. PR #320 is merged as `750ac3e`; its exact NRO transfers with nxlink exit
0 at 08:47:45 UTC and the new reports now accept its first observed load.

The [PixModeSync bridge](../docs/GX_PIX_MODE_SYNC_2026-10-06.md) passes 21 suites,
six rejected mutants, both SDK modes, full synthetic/private rendered builds
and five workflows / six jobs on code and final PR HEAD. PR #318 is merged
as `7b41350`. The exact 73,556,024-byte NRO transfers with nxlink exit 0 at
19:14:18 UTC; 65 strong functions and 39 scoped unique providers are verified.
The preceding heartbeat retains 5,988 FIFO writes and 102 successful presents /
zero failures; it does not identify pixels. The getter requires the saved guest viewport and its pinned frame-gated
offscreen-screen side effect.
The [viewport/depth candidate](../docs/GX_VIEWPORT_STATE_2026-10-06.md) implements
that getter and the statically checked depth dependency. It passes 22 local
suites, six compiled rejected mutants, the SDK/synthetic/private builds and
all five code-head workflows / six jobs. Its exact 73,621,560-byte NRO is
verified on the SD card by complete USB readback. PR #319 is merged as
`378267f` after all five final-head workflows / six jobs. The exact NRO
transfers via Netloader with exit 0 at 2026-10-07 07:53:15 UTC (26,799,100
compressed bytes / 2,253 blocks). The user reports a black screen with the
test still running; fresh reports now accept the observed getter/depth return
and establish the guarded I4 load frontier.

The [bounded copy](../docs/GX_COPY_TEX_2026-10-06.md) remains accepted for RGB5A3
128×128, clear 1, now also at destination `0x9210A740`. GPU lifetime tests
remain broader than this observed execution. PR #317 is merged as `e5ec490`.

The [configuration bridges](../docs/GX_TEXTURE_COPY_CONFIG_2026-10-06.md) remain
accepted for Clamp(3), Src(0,0,128,128), Dst(128,128,5,0). PR #316 is merged
as `666e559`.
The [Fog correction](../docs/GX_FOG_DEGENERATE_2026-10-05.md) passed all eighteen
local suites, both AArch64 modes, the synthetic/private rendered builds and
five exact-head workflows / six jobs. PR #315 is merged as `f6da5a7`.
Its exact NRO transferred via nxlink with exit 0 at 2026-10-06 04:57:43 UTC;
the new reports now accept its observed Mii tuple return.

The [PADReset candidate](../docs/PAD_RESET_2026-10-05.md), code `7ac668e`, passed
5,223 cases, eighteen local suites, five exact-code workflows / six jobs and
its private rendered build. Its exact NRO transferred via direct nxlink with
exit 0 at 20:07:42 UTC on October 5. The preceding
[motor run](../docs/HARDWARE_RESULTS_2026-10-05_PAD_CONTROL_MOTOR_PAD_RESET_FRONTIER.md)
accepts channel 0 / command 2 before the now-crossed reset boundary.

The [PADControlMotor correction](../docs/PAD_CONTROL_MOTOR_2026-10-05.md) passed
4,152 host cases, seventeen local suites, five exact-code workflows / six jobs
and its private NRO build. The exact NRO transferred via nxlink with exit 0
at 17:13:07 UTC on October 5. The [preceding KPAD run](../docs/HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md)
accepts count-1 polling; larger counts and raw guest-output capture remain open.

The [WPADProbe bridge](../docs/WPAD_PROBE_2026-10-04.md) passes 556 host cases,
six mutations, five exact-code workflows / six jobs and its private NRO build.
Its exact NRO transferred successfully at 19:44:01 UTC. The preceding
[PADRead bridge](../docs/PAD_READ_2026-10-04.md) remains crossed through translated
PADClampCircle2. The earlier snapshot retains 3,937 FIFO writes, 99 preceding
successful presents and zero replay calls. Per-button input remains open.

The [SU-state correction](../docs/GX_SU_STATE_2026-10-04.md) passes 632 host cases /
39 refusals, all five exact-code GitHub workflows / six jobs and its private
NRO build. The [earlier pending-state refusal](../docs/HARDWARE_RESULTS_2026-10-04_DISPLAY_LIST_PENDING_STATE.md)
remains the diagnostic baseline, not the current frontier.

The [depth-LOD fix](../docs/GX_DEPTH_LOD_2026-10-03.md) passes local/native
contracts, all five exact-code workflows / six jobs and its private NRO build.
The first transfer failed; the later retry completed with exit 0 at
**20:49:11 UTC** on 2026-10-03. Fresh verified reports now accept the observed
LOD return. The earlier IA8/matrix/coordinate/TEV/AlphaCompare/Fog/ZCompLoc and
pixel-setup progression remains crossed; alternate inputs retain host proof.

The preceding heartbeat at dispatch 608302 records 1556 guest FIFO writes,
99 successful presents and zero failures. A later post-main snapshot at
608722, before the texture constructor, records 1558 FIFO writes, last word
`E8000156`, and the same 99 presents. These counters do not establish a frame
with visible content after LOD or separately count native BP emissions.
The watchdog has 102 ACTIVE and one recovered STALE sample, maximum interval
2168 ms. Discovery first hits, checked caller flow and later coherent state
establish returns without tracing every invocation.

The [bounded coordinate candidate](../docs/GX_TEX_COORD_BATCH_2026-10-03.md),
code `91a4a01b8e316f9010e9d31754e279065772f9f3`, passed its local host/workflow
checks and private Rendered Discovery build. Its 73,297,976-byte NRO has
SHA-256 `64ba837720f4e37cbd127c37a0e9bde6dc146ed229a92c8697b9c531a8984d08`.
Nxlink transferred it with exit 0 at 2026-10-02 23:09:38 UTC (01:09:38 on
October 3, Europe/Paris). Fresh reports now accept the eight exact triples;
enabled Scale/Bias branches still have host contracts only. The user confirmed
a black screen for that coordinate run. Its Direct stage-0 arrival remained
unreturned until the later TEV run described below.

The separate audit NRO has SHA-256
`7ecbc8a9fe1efb31697c2ee36d0b0b648a8e87d3fa7dda1fb9d262d6de5b7d09`.
Nxlink transferred it with exit 0 at 2026-10-03 09:25:19 UTC. Fresh reports
accept normal-path non-regression through the same TEV Direct frontier;
the user again confirmed a black screen. SIZE_MAX rejection, Present(false),
teardown exceptions and shutdown recovery were not exercised on this run.
See [the audit record](../docs/PORT_AUDIT_2026-10-03.md).

The [bounded TEV scalar batch](../docs/GX_TEV_SCALAR_BATCH_2026-10-03.md) passed
local contracts, all five GitHub workflows on integrated code `e76e8f38`, and
its exact private Rendered Discovery build. NRO SHA-256 `cc88a78c...` transferred
with exit 0 at 2026-10-03 11:39:21 UTC. Fresh reports accept all sixteen
iterations on the caller default tuples; alternate arguments retain host
contracts only. The user saw a black screen and an error at the end. The
following KColor guest-pointer boundary stays outside this accepted lot.
The [TEV color/table batch](../docs/GX_TEV_COLOR_BATCH_2026-10-03.md)
implements KColor plus the audited adjacent Color and SwapModeTable setters.
All five GitHub workflows, its ten host contracts, rendered syntax gate and
exact private build pass on code `1333b0e2`. NRO `a56be881...` transferred
with exit 0 at 13:43:58 UTC; fresh reports now accept all twelve executed calls.
The next [AlphaCompare candidate](../docs/GX_ALPHA_COMPARE_2026-10-03.md) implements
the observed hard stop and preserves the existing native validity flag.
Its eleven host contracts, rendered syntax, all five GitHub workflows and
exact private build pass on code `1a8c092f`. NRO `7032c756...` is ready with
27 checked symbols and unique native/flag providers. Nxlink transferred it
with exit 0 at 17:50:16 UTC; fresh reports accept the observed AlphaCompare
return and identify Fog as the next hard stop.

The method now permits bounded GX batches after auditing the pinned wrapper,
Aurora effects, argument guards and relevant guest-memory mirrors. Every
member still requires its own progression proof; unknown/stateful calls
remain hard stops. Older texture-object, KD and audio results are dated
historical evidence, and different scheduler paths can expose different gates.
A visually correct Mario Kart Wii image is still unproven. The preceding
IA8, coordinate, audit and TEV runs were observed black.

The complete blocker-by-blocker history and current checklist live in
[`ROADMAP.md`](../ROADMAP.md). Hardware evidence is recorded in dated files under
[`docs/`](../docs/). Static look-ahead is available through
`scripts/forecast-rmcp01-frontier.py`. For whole-product coverage and
first-hit runtime tracing, see
[`docs/RMCP01_DISCOVERY_SCAN.md`](../docs/RMCP01_DISCOVERY_SCAN.md) and build:

```sh
MKW_JOBS=4 bash scripts/build-local-rendered-discovery-scan.sh
```

Discovery mode still hard-stops on unknown/stateful boundaries; hardware
evidence remains the authority for runtime patches.

## Roadmap and evidence

Start with:

- [TEV scalar hardware result](../docs/HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md) — six setters on stages 0..15 return; KColor pointer frontier, black screen and error at exit;
- [TEV scalar batch](../docs/GX_TEV_SCALAR_BATCH_2026-10-03.md) — legal SDK domains, host/private-build validation and bounded hardware scope;
- [Earlier audit result](../docs/HARDWARE_RESULTS_2026-10-03_AUDIT_GX_TEV_DIRECT_FRONTIER.md) — normal-path non-regression at the preceding Direct frontier;
- [Accepted coordinate batch](../docs/GX_TEX_COORD_BATCH_2026-10-03.md) — eight exact triples, host/private-build validation, exact NRO and enabled-branch limits;
- [`ROADMAP.md`](../ROADMAP.md) — authoritative current milestone/frontier checklist;
- [`docs/FAST_TRACK_VALIDATION_POLICY.md`](../docs/FAST_TRACK_VALIDATION_POLICY.md) — required validation ladder, strict hardware-cross definition, invariant checklist, and private rendered-build gate;
- [`docs/M2_RUNTIME_BOOTSTRAP.md`](../docs/M2_RUNTIME_BOOTSTRAP.md) — current runtime/bootstrap architecture and hardware method;
- [`docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`](../docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md) — first real `main()` proof;
- [`docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`](../docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md) — HostContext guest continuation proof;
- [`docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`](../docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md) — scheduler frontier that preceded sustained execution;
- [`docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`](../docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md) — first sustained black-screen / AsyncDisplay evidence;
- [`docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md`](../docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md) — 126,563-dispatch run proving the black-screen path is an active VI/post-retrace loop;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md`](../docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md) — real-Switch changing-color NVK/VI clear-frame presentation proof;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md`](../docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_TRIANGLE.md) — real-Switch Vulkan shader/pipeline/rasterisation triangle proof and SD-report follow-up;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md`](../docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md) — real-Switch Aurora GX triangle proof, 563-frame active loop, and clean teardown;
- [`docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md`](../docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md) — real-Switch exact pinned `HleFifoWrite` → Aurora GX proof with a 1,435-frame active loop;
- [`docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md) — FST publication PASS, #185 DVD-read non-reachability, and later priority-6 thread frontier;
- [`docs/HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md`](../docs/HARDWARE_RESULTS_2026-09-19_VI_POLL_CONTEXT_REGRESSION.md) — #188 first-fiber crash attribution and register-isolation fix gate;
- [`docs/HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_OS_SEND_MESSAGE_FRONTIER.md) — #189 hardware PASS, VI starvation fixed, and new `OSSendMessage` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_GX_DRAW_DONE_FRONTIER.md) — #190 OSSendMessage PASS and new `GXDrawDone` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_TASK_THREAD_FRONTIER.md) — #191 GXDrawDone PASS and resource `TaskThread::run` frontier;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_GX_SET_PROJECTION_FRONTIER.md) — first #192 hardware run, TaskThread validation caveat, and new PAL `GXSetProjection` blocker;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_GX_SET_VIEWPORT_FRONTIER.md) — #193 hardware-proves TaskThread + projection and exposes PAL `GXSetViewport`;
- [`docs/HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-19_GX_SET_SCISSOR_FRONTIER.md) — #194 hardware-proves `GXSetViewport` and exposes PAL `GXSetScissor`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_LOAD_POS_MTX_IMM_FRONTIER.md) — #195 hardware-proves `GXSetScissor` and exposes PAL `GXLoadPosMtxImm`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CURRENT_MTX_FRONTIER.md) — #196 hardware-proves `GXLoadPosMtxImm`, re-proves `TaskThread::run`, and exposes PAL `GXSetCurrentMtx`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_CLEAR_VTX_DESC_FRONTIER.md) — #197 hardware-proves `GXSetCurrentMtx` and exposes PAL `GXClearVtxDesc`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_DESC_FRONTIER.md) — #198 hardware-proves `GXClearVtxDesc`, re-proves TaskThread, records first post-bootstrap GX FIFO state traffic, and exposes PAL `GXSetVtxDesc`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_VTX_ATTR_FMT_FRONTIER.md) — #199 hardware-proves `GXSetVtxDesc` and exposes PAL `GXSetVtxAttrFmt`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_NUM_CHANS_FRONTIER.md) — #200 hardware-proves `GXSetVtxAttrFmt`, explains the deliberate abort/return-to-hbmenu behavior, and exposes PAL `GXSetNumChans`;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_MAT_COLOR_FRONTIER.md) — #201 hardware-proves `GXSetNumChans`, re-proves TaskThread, and exposes PAL `GXSetChanMatColor`;
- [`docs/HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-22_GX_FLUSH_CROSSED_TASK_THREAD_JOB_FRONTIER.md) — hardware-crosses `GXFlush` with 23 successful presents and records the current TaskThread job-dispatch diagnostic frontier;
- [`docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_STACK_JOB_FRONTIER.md) — proves the received TaskThread “job” aliases the worker stack and moves the frontier to send-side / queue-buffer source attribution;
- [`docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-22_TASK_THREAD_VALID_SEND_RECEIVE_SLOT_FRONTIER.md) — proves `TaskThread::request` sends the valid `mJobs[0]` pointer and moves the frontier to the exact `OSReceiveMessage` output-slot clobber phase;
- [`docs/HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md`](../docs/HARDWARE_RESULTS_2026-09-23_VI_POLL_INTERRUPT_MASK_TASK_THREAD_FIX.md) — phase trace proves the receive slot is correct until the `OSWakeupThread` call boundary; attributes the clobber to Switch pre-call VI retrace delivery while guest interrupts are disabled and defines the minimal interrupt-mask fix;
- [`docs/HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-23_TASK_THREAD_DVD_READ_IDLE_FRONTIER.md) — hardware-validates the interrupt-mask fix, records the first real `/Boot/Strap/eu/English.szs` read-pass, and moves the frontier to `SELECTTHREAD_IDLE_POLL`;
- [`docs/HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-23_ASYNC_DISPLAY_IDLE_VI_WAKE_FRONTIER.md) — attributes the idle blocker to `AsyncDisplay::syncTick` and the required wake to VI `PostRetraceCallback`, defining the VI-only scheduler idle candidate;
- [`docs/HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-24_EGG_DECOMP_SZS_FRONTIER.md) — hardware-validates AsyncDisplay idle recovery, recovers the real GPU present path, and moves the frontier to pinned `EGG::Decomp::decodeSZS (0x80218C2C)`;
- [`docs/HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-24_SZS_CROSSED_GX_INIT_TEX_OBJ_FRONTIER.md) — hardware-validates complete `English.szs` Yaz0 expansion and moves the exact frontier to `GXInitTexObj (0x801707F8)`;
- [`docs/HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-24_GX_INIT_TEX_OBJ_CROSSED_IOS_OPEN_FRONTIER.md) — hardware-crosses `GXInitTexObj`, preserves one successful GPU present, and moves the exact frontier to `NAND_IOS_Open (0x801938F8)` with path diagnostics only;
- [`docs/HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-24_IOS_OPEN_KD_REQUEST_FRONTIER.md) — identifies the exact IOS path as `/dev/net/kd/request`, mode 0, and defines the minimal pinned device-handle open candidate;
- [`docs/HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-24_KD_OPEN_CROSSED_IOS_IOCTL_CMD2_FRONTIER.md) — hardware-crosses the KD open, captures the full fd/cmd/in/out tuple for `IOS_Ioctl (0x80194290)` command 2, and defines the one-shot Boot-phase `-42` reply candidate;
- [`docs/HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-25_KD_CMD2_CROSSED_IOS_CLOSE_FRONTIER.md) — hardware-crosses the first KD command-2 Boot probe and moves the exact frontier to `IOS_Close (0x80193AD8)` with fd 2000;
- [`docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-20_GX_SET_CHAN_CTRL_FRONTIER.md) — merged #203 hardware-proves `GXSetChanMatColor`, preserves the scheduler/GX chain, and exposes PAL `GXSetChanCtrl`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEX_GENS_FRONTIER.md) — real Switch progresses beyond `GXSetChanCtrl` and exposes PAL `GXSetNumTexGens`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_IND_STAGES_FRONTIER.md) — real Switch progresses beyond `GXSetNumTexGens` and exposes PAL `GXSetNumIndStages`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_NUM_TEV_STAGES_FRONTIER.md) — real Switch progresses beyond `GXSetNumIndStages`, advances FIFO state traffic, and exposes PAL `GXSetNumTevStages`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_OP_FRONTIER.md) — real Switch progresses beyond `GXSetNumTevStages` and exposes PAL `GXSetTevOp`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_TEV_ORDER_FRONTIER.md) — real Switch progresses beyond `GXSetTevOp` and exposes PAL `GXSetTevOrder`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_BLEND_MODE_FRONTIER.md) — real Switch progresses beyond `GXSetTevOrder` and exposes PAL `GXSetBlendMode`;
- [`docs/HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md`](../docs/HARDWARE_RESULTS_2026-09-21_GX_SET_COLOR_UPDATE_FRONTIER.md) — real Switch progresses beyond `GXSetBlendMode` and exposes PAL `GXSetColorUpdate`;
- [`docs/M3_RMCP01_RENDERED_FAST_TRACK.md`](../docs/M3_RMCP01_RENDERED_FAST_TRACK.md) — first local Mario Kart graphics-enabled fast-track.

Older dated `HARDWARE_RESULTS_*` files are historical snapshots. Their “next blocker” wording intentionally reflects what was known on that date and is not rewritten retroactively.

## Upstream

The audited WiiCompiled revision is pinned to:

```text
a135beb201042b20f390c6695ca6b26768820fb4
```

CI enforces the pin. New hardware blockers are mapped against that exact revision before any HLE behavior is added.

## 2026-10-08 — First real frame captured and replayed

The opt-in Switch recorder completed the initial RMCP01 frame. Desktop Aurora
replay and GPU readback succeeded in two independent processes with identical
PNGs: one untextured quad and uniformly black output. Later textured frames and
recognizable game pixels remain unproven. Fresh runtime reports record 102
successful presents before the guarded Mii I4 texture-load frontier.
See the [scoped hardware result](HARDWARE_FIRST_FRAME_REPLAY_2026-10-08.md).

## 2026-10-08 — Second next-pass I4 return

The bounded second next-pass Mii I4 correction passes local contracts,
required HLE CI and the private rendered build. A fresh Switch run accepts
its native/helper return through the checked caller sequence, then stops at
an unadmitted RGB5A3 44×32 object. Runtime invariants and 102 successful
presents remain coherent; the initial capture/PNG is unchanged. See the
[hardware result](HARDWARE_SECOND_NEXT_PASS_I4_RETURN_2026-10-08.md).

## 2026-10-08 — Next-pass RGB5A3 candidate validated

The [observed 44×32 RGB5A3 correction](GX_MII_RGB5A3_NEXT_PASS_LOAD_2026-10-08.md)
passes all 22 local suites, five focused mutations, six GitHub workflows / seven
jobs and the private rendered/capture build. The complete SD copy is verified.
It has not been launched; native return and visual progress remain pending.

## 2026-10-08 — Next-pass RGB5A3 return

The corrected RGB5A3 native load, draw helper and intervening state calls return
through the checked caller sequence. All 38 reports are verified, thirteen changed
and twenty-five retained. The new guarded stop is I4 36×32 object `0x80397EC0`,
slot 0, source `0x109C1780`. Runtime invariants and 102 successful presents remain
coherent; the initial black capture/PNG is unchanged. See the
[hardware result](HARDWARE_NEXT_PASS_RGB5A3_RETURN_2026-10-08.md).
