# Roadmap

## Current checkpoint — 2026-10-08

The [corrected GPU surface capture](docs/HARDWARE_SURFACE_CHECKPOINT_2026-10-08.md)
saves the first and last completed frames on Switch. Both independently decoded
1280×720 images have black RGB; the first is opaque and the last has zero alpha.
The latest-image replacement and diagnosed-stop checkpoint are hardware-validated.
Execution stops at the next guarded I4 36×32 identity after the previous load/helper.
Recognizable game rendering remains unproven. The initial FIFO capture remains
identical to the preceding black desktop replay.

The [texture-family, presentation and sequence-replay candidate](docs/GX_TEXTURE_FAMILY_PRESENT_REPLAY.md)
removes object/source address admission for the bounded I4/RGB5A3 family, separates
the persistent EFB from the surface, presents the selected XFB and records complete
frame prefixes. Its synthetic checks pass; console validation is pending.

- [x] Implement the [coordinated GX display-list candidate](docs/GX_DISPLAY_LIST_2026-10-03.md): shared checked native/guest buffer, Begin/End and context restoration.
- [x] Validate final display-list code `6fb2718b`: 335 rendered cases / 30 diagnosed refusals, both headless refusals, four rejected mutants, five GitHub workflows / six jobs and the private NRO build.
- [x] Transfer the exact display-list NRO with exit 0 at 2026-10-04 09:19:26 UTC; verify 29 reports / 534,803 bytes and the pending guest SU-state guard.
- [x] Implement the [bounded SU-state correction](docs/GX_SU_STATE_2026-10-04.md): actual native texture register emission, selective guest shadow publication and Begin/End ordering; 632 host cases / 39 refusals pass.
- [x] Pass SU correction `4a7e48b5` local contracts, six mutations, five exact-code GitHub workflows / six jobs and private NRO build; thirteen scoped providers verified.
- [x] Transfer the SU NRO with exit 0 at 10:01:32 UTC, verify 30 reports / 535,712 bytes and accept native empty-update SU processing plus Begin/End return and a later 64-byte allocation length.
- [x] Capture GXDrawSphere `0x80172A30`, `(4,8)`, dispatch 607503 / 104.178 seconds.
- [x] Implement the [bounded sphere bridge](docs/GX_DRAW_SPHERE_2026-10-04.md), execute its native recording dependencies and fix the demonstrated float conversion bug; 648 cases / 48 refusals pass.
- [x] Pass sphere code `e7dd6806` on thirteen local suites, four rejected mutants, five exact-code workflows / six jobs and the private NRO build; sixteen scoped providers verified.
- [x] Hardware-accept both sphere variants returning; verify 31 reports / 541,214 bytes and the PADRead frontier.
- [x] Implement the [PADRead candidate](docs/PAD_READ_2026-10-04.md) using the existing Switch input service; 65,563 host cases pass.
- [x] Pass PADRead code `7ca14a76` on fourteen local suites, five rejected mutants, five exact-code workflows / six jobs, the full local synthetic build and private rendered NRO build; seventeen scoped providers verified.
- [x] Transfer PADRead with exit 0 at 14:44:46 UTC; verify 32 reports / 541,985 bytes, connected port 0 and PADRead return through PADClampCircle2.
- [x] Implement the [WPADProbe absent-remote candidate](docs/WPAD_PROBE_2026-10-04.md); 556 host cases pass with preserved CPU, memory and library state.
- [x] Validate WPADProbe code `2941f1d`: fifteen local suites, six rejected mutants, five exact-code workflows / six jobs, full synthetic and private NRO builds; 47 retained symbols / 18 scoped providers.
- [x] Transfer WPADProbe with exit 0 at 19:44:01 UTC; independently verify 33 reports / 542,797 bytes and accept channel 0 return before KPADGetUnifiedWpadStatus `0x8019812C`, count 1.
- [x] Implement the [KPAD unified status candidate](docs/KPAD_UNIFIED_STATUS_2026-10-04.md): complete absent samples and count limit 16; 8,987 host cases pass.
- [x] Pass KPAD unified local validation: sixteen suites, eight mutants, both AArch64 modes, full synthetic build and script/workflow lint.
- [x] Pass all five exact-code KPAD workflows / six jobs and the private rendered NRO build; verify 48 retained symbols and nineteen scoped providers.
- [x] Transfer the exact KPAD unified NRO with exit 0 at 21:42:13 UTC (26,743,208 compressed bytes / 2,247 blocks).
- [x] Retrieve and verify 34 KPAD-run reports / 544,124 bytes; accept count-1 polling return through later caller progression and the final channel-3 report.
- [x] Capture [PADControlMotor `0x801AF908`](docs/HARDWARE_RESULTS_2026-10-05_KPAD_UNIFIED_PAD_CONTROL_MOTOR_FRONTIER.md), channel 0, command 2, dispatch 618177 / 109.316 seconds; user confirms black then error.
- [x] Implement the [PADControlMotor absent-actuator bridge](docs/PAD_CONTROL_MOTOR_2026-10-05.md), preserving the void CPU ABI and zero rumble capability.
- [x] Validate PADControlMotor code `1cff562`: 4,152 host cases, four rejected mutants, seventeen local suites, synthetic retention, both AArch64 modes, five exact-code workflows / six jobs and private rendered build; 49 strong symbols and twenty scoped providers verified.
- [x] Copy the validated PADControlMotor NRO to the Switch over USB/MTP at 05:27:43 UTC; independently verify all transferred bytes and SHA-256 by complete readback.
- [x] Transfer the exact PADControlMotor NRO via direct nxlink with exit 0 at 17:13:07 UTC (26,743,203 compressed bytes / 2,247 blocks); candidate hashes and dependency pins reverified.
- [x] Retrieve and verify [35 reports / 544,876 bytes](docs/HARDWARE_RESULTS_2026-10-05_PAD_CONTROL_MOTOR_PAD_RESET_FRONTIER.md); accept motor channel 0 / command 2 return before PADReset `0x801AF0DC`, mask `0x70000000`, dispatch 619288 / 112.024 seconds.
- [x] Implement the [PADReset pinned ignored-mask success contract](docs/PAD_RESET_2026-10-05.md); 5,223 executable host cases pass.
- [x] Pass PADReset code `7ac668e` on eighteen local suites, four rejected mutations, both AArch64 modes, full synthetic build, five exact-code workflows / six jobs and private Rendered Discovery build; 50 strong symbols / 21 scoped providers verified.
- [x] Copy the exact PADReset NRO to the SD at 17:55:43 UTC; complete readback verifies all 73,494,584 bytes and SHA-256.
- [x] Launch the exact PADReset NRO via nxlink with exit 0 at 20:07:42 UTC; verify [36 reports / 629,680 bytes](docs/HARDWARE_RESULTS_2026-10-05_PAD_RESET_MII_FOG_FRONTIER.md) and accept mask `0x70000000` return before the new `(1,1,0,0)` Fog refusal in Mii texture preparation.
- [x] Implement the [bounded second Fog tuple](docs/GX_FOG_DEGENERATE_2026-10-05.md), preserving native coefficient/BP work and whole-tuple guards.
- [x] Validate the second Fog tuple: 67,661 valid calls / 630 refusals per mode, 2,048 native BP fixtures, four rejected mutants, all eighteen local suites, both AArch64 modes, full synthetic and private rendered builds; 50 strong symbols / 23 scoped providers verified. Remote acceptance is tracked in [PR #315](https://github.com/yashin-sh/WiiCompiled-Switch/pull/315).
- [x] Merge Fog PR #315 (`f6da5a7`) after all five exact-head workflows / six jobs; copy the exact NRO to SD with full readback.
- [x] Launch Fog via nxlink with exit 0 at 2026-10-06 04:57:43 UTC; verify [36 reports / 628,989 bytes](docs/HARDWARE_RESULTS_2026-10-06_MII_FOG_COPY_CLAMP_FRONTIER.md) and accept the `(1,1,0,0)` Mii tuple returning before GXSetCopyClamp `0x8016F618`, value 3, dispatch 631958 / 119.749 seconds.
- [x] Implement the [bounded texture-copy configuration lot](docs/GX_TEXTURE_COPY_CONFIG_2026-10-06.md): observed Clamp and guest mirrors; checked-caller Src/Dst pre-ports with pinned HLE shadow.
- [x] Validate configuration code `fec006a`: 393,253 rendered calls / 12 diagnosed refusals, 12 headless refusals, 524,292 native fixtures, seven rejected mutants, nineteen local suites, both AArch64 modes, full synthetic/private rendered builds and five exact-code workflows / six jobs; 56 retained strong functions and 30 scoped providers verified.
- [x] Copy the 73,498,680-byte configuration NRO to SD at 05:50:54 UTC with complete readback; the first direct nxlink attempt fails before transfer. PR #316 merges as `666e559` at 06:05:01 UTC after final documentation-head checks.
- [x] Transfer the exact configuration NRO via direct nxlink with exit 0 at 16:47:54 UTC (26,745,240 compressed bytes / 2,247 blocks).
- [x] Retrieve and independently verify 36 reports / 630,085 bytes; accept Clamp(3), TexCopySrc(0,0,128,128) and TexCopyDst(128,128,5,0) return.
- [x] Capture GXCopyTex `0x8016FD74`, destination `0x9210A720`, clear 1, dispatch 633774 / 126.018 seconds.
- [x] Implement the [bounded GXCopyTex candidate](docs/GX_COPY_TEX_2026-10-06.md), native copy/draw ordering, full 32 KiB preflight and destination retirement via DC/DMA hooks.
- [x] Pass GXCopyTex local validation: 65,746 calls / 25 rendered refusals, two headless refusals, 65,025 pinned size cases, nine rejected mutants, twenty suites, both AArch64 modes and full synthetic retention.
- [x] Pass GXCopyTex code `c18f257` on five exact-code workflows / six jobs and the private rendered NRO build; verify 63 strong functions and 37 scoped providers.
- [x] Copy the exact 73,543,736-byte GXCopyTex NRO to SD at 18:18:20 UTC with complete byte/hash readback.
- [x] Transfer and launch the exact GXCopyTex NRO via direct nxlink, exit 0 at 18:27:37 UTC, 26,769,071 compressed bytes / 2,249 blocks.
- [x] Verify 37 fresh reports / 629,452 bytes, 13 changed / 24 identical; accept the observed native copy return via copy-pass plus later GXPixModeSync stop.
- [x] Implement the [faithful GXPixModeSync candidate](docs/GX_PIX_MODE_SYNC_2026-10-06.md); pass 21 local suites, six rejected mutants, both SDK modes, full synthetic retention and five exact-code workflows / six jobs.
- [x] Build the exact PixModeSync rendered NRO (73,556,024 bytes), verify 65 strong functions and 39 scoped unique providers; preserve candidate bytes and upstream patch/mtimes.
- [x] Transfer the exact PixModeSync NRO via direct nxlink, exit 0 at 19:14:18 UTC, 26,774,763 compressed bytes / 2,249 blocks.
- [x] Verify 37 reports / 631,739 bytes and accept PixModeSync return through later Mii code and GXGetViewportv; merge #318 after all final-head jobs pass.
- [x] Implement the [viewport snapshot/getter and audited depth dependency](docs/GX_VIEWPORT_STATE_2026-10-06.md).
- [x] Pass 22 suites, six rejected mutants, all 65 SDK sources, synthetic retention, five code-head workflows / six jobs and the private rendered build; verify 69 strong functions / 46 scoped providers.
- [x] Deploy the exact 73,621,560-byte viewport/depth NRO and verify every SD byte by USB readback.
- [x] Merge viewport PR #319 as `378267f` after all five final publication-head workflows / six jobs.
- [x] Transfer the exact viewport/depth NRO via Netloader with exit 0 at 2026-10-07 07:53:15 UTC, 26,799,100 compressed bytes / 2,253 blocks; the user reports black with the test still running.
- [x] Verify [37 reports / 630,954 bytes](docs/HARDWARE_RESULTS_2026-10-07_VIEWPORT_MII_I4_LOAD_FRONTIER.md), 16 changed / 21 retained; accept observed getter/depth return before GXLoadTexObj at object `0x80397D80`, slot 0, I4 32×64.
- [x] Implement and pass targeted contracts for the [bounded Mii I4 load](docs/GX_MII_I4_LOAD_2026-10-07.md).
- [x] Validate I4 code `076dce6`: 22 local suites, six rejected mutants, both SDK modes, full synthetic/private rendered builds; 71 strong functions and 48 scoped providers verified. The 73,621,560-byte NRO has SHA-256 `e7cc019d...`; final-head CI, merge and deployment evidence are tracked in [PR #320](https://github.com/yashin-sh/WiiCompiled-Switch/pull/320).
- [x] Merge first I4 PR #320 (`750ac3e`) after all five final-head workflows / six jobs; deploy with complete readback and transfer via Netloader with exit 0 at 08:47:45 UTC.
- [x] Verify [37 reports / 632,060 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_LOAD_FRONTIER.md), 13 changed / 24 retained; accept first I4 load and intervening helper return before the second object `0x80397DC0`.
- [x] Implement and pass targeted contracts for the [second captured I4 object](docs/GX_MII_I4_SECOND_LOAD_2026-10-07.md).
- [x] Validate second-object code `3256e81`: 22 local suites, eight rejected mutants, rendered SDK gate, full synthetic/private builds; 71 strong functions and 48 scoped providers verified. The 73,621,560-byte NRO has SHA-256 `fe2a28d9...`; final-head CI, merge and verified deployment are tracked in [PR #321](https://github.com/yashin-sh/WiiCompiled-Switch/pull/321).
- [x] Merge second I4 PR #321 (`4da5100`) after all five final-head workflows / six jobs; deploy with complete readback and transfer via Netloader with exit 0 at 09:42:22 UTC.
- [x] Verify [37 reports / 633,035 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_LOAD_FRONTIER.md), 13 changed / 24 retained; accept second I4 load and intervening helper/setup return before RGB5A3 object `0x80397D40`, slot 0, 44×32.
- [x] Implement and pass targeted contracts for the [captured RGB5A3 load](docs/GX_MII_RGB5A3_LOAD_2026-10-07.md).
- [x] Validate RGB5A3 code `e6a4727`: 22 local suites, twelve rejected mutants, rendered SDK gate, full synthetic/private builds; 71 strong functions and 48 scoped providers verified. The 73,621,560-byte NRO has SHA-256 `4794cdc6...`; final-head CI, merge and verified deployment are tracked in [PR #322](https://github.com/yashin-sh/WiiCompiled-Switch/pull/322).
- [x] Merge RGB5A3 PR #322 (`8938122`) after all five final-head workflows / six jobs; deploy with complete readback and transfer via Netloader with exit 0 at 10:17:06 UTC.
- [x] Verify [37 reports / 632,748 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_LOAD_FRONTIER.md), 13 changed / 24 retained; accept RGB5A3 and intervening draw-helper return before I4 object `0x80397CC0`, slot 0, 36×32.
- [x] Implement and pass targeted contracts for the [captured I4 36×32 load](docs/GX_MII_I4_36X32_LOAD_2026-10-07.md).
- [x] Validate I4 36×32 code `4056328`: 22 local suites, seventeen rejected mutants, rendered SDK gate, full synthetic/private builds; 71 strong functions and 48 scoped providers verified. The 73,621,560-byte NRO has SHA-256 `e2ae9043...`; final-head CI, merge and verified deployment are tracked in [PR #323](https://github.com/yashin-sh/WiiCompiled-Switch/pull/323).
- [x] Merge first I4 36×32 PR #323 (`c46f2ad`) after all five final-head workflows / six jobs; deploy with full readback and transfer through Netloader with exit 0 at 13:32:04 CEST.
- [x] Verify [37 reports / 632,902 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_36X32_SECOND_LOAD_FRONTIER.md), 16 changed / 21 retained; accept first 36×32 load/helper return before identical second object `0x80397D00`, slot 0.
- [x] Implement and pass targeted contracts for the [second I4 36×32 object](docs/GX_MII_I4_36X32_SECOND_LOAD_2026-10-07.md).
- [x] Validate second-36×32 code `5e65cdd`: 22 suites, nineteen rejected mutants, rendered SDK gate, full synthetic/private builds; 71 strong functions and 48 scoped providers verified. The 73,621,560-byte NRO has SHA-256 `bf093aea...`; final-head CI, merge and verified deployment are tracked in [PR #324](https://github.com/yashin-sh/WiiCompiled-Switch/pull/324).
- [x] Merge PR #324 (`1df611f`) after five final-head workflows / six jobs; verify full SD readback and successful Netloader transfer at 14:26:06 CEST.
- [x] Verify [37 reports / 632,811 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_38X32_LOAD_FRONTIER.md), seven changed / thirty retained; accept second-36×32 and intervening-helper return before RGB5A3 `0x80397C40`, slot 0, 38×32.
- [x] Implement and pass targeted contracts for the [captured RGB5A3 38×32 correction](docs/GX_MII_RGB5A3_38X32_LOAD_2026-10-07.md): 24 loads per mode, 2,164 / 2,180 diagnosed refusals.
- [x] Reject 25 compiled mutations; pass SDK/synthetic/private builds and lint on code `ad75948`, retaining 71 strong functions and 48 scoped unique providers. NRO 73,621,560 bytes, SHA-256 `4fd7e46e...`; the 22-suite gate, final-head CI, merge and verified SD deployment are tracked in [PR #326](https://github.com/yashin-sh/WiiCompiled-Switch/pull/326).
- [x] Merge PR #326 (`101fee4`) after five final-head workflows / six jobs; verify SD readback and successful Netloader transfer at 15:07:34 CEST.
- [x] Verify [37 reports / 632,323 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_RGB5A3_38X32_SECOND_LOAD_FRONTIER.md), sixteen changed / twenty-one retained; accept first-38×32 and helper return before identical second object `0x80397C80`, slot 0.
- [x] Implement and pass targeted contracts for the [second RGB5A3 38×32 object](docs/GX_MII_RGB5A3_38X32_SECOND_LOAD_2026-10-07.md): 26 loads per mode, 2,436 / 2,454 diagnosed refusals.
- [x] Reject 27 compiled mutations; pass SDK/synthetic/private builds and lint on code `63df50a`, retaining 71 strong functions and 48 scoped unique providers. NRO 73,621,560 bytes, SHA-256 `ccbf2569...`; the 22-suite gate, final-head CI, merge and verified SD deployment are tracked in [PR #327](https://github.com/yashin-sh/WiiCompiled-Switch/pull/327).
- [x] Merge PR #327 (`0d4fff0`) after five final-head workflows / six jobs; verify SD readback and successful Netloader transfer at 15:40:26 CEST.
- [x] Verify [37 reports / 632,976 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_16X16_LOAD_FRONTIER.md), thirteen changed / twenty-four retained; accept second-RGB5A3/helper/state-call return before I4 `0x80397E00`, slot 0, 16×16.
- [x] Implement and pass targeted contracts for the [captured I4 16×16 object](docs/GX_MII_I4_16X16_LOAD_2026-10-07.md): 28 loads per mode, 2,708 / 2,728 diagnosed refusals.
- [x] Pass SDK/synthetic/private builds and lint on code `e0dd8e8`, retaining 71 strong functions and 48 scoped unique providers. NRO 73,621,560 bytes, SHA-256 `75686e0f...`; 33 mutation checks, the 22-suite gate, final-head CI, merge and verified SD deployment are tracked in [PR #328](https://github.com/yashin-sh/WiiCompiled-Switch/pull/328).
- [x] Establish [I4 16×16 and prior-caller-pass return](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_32X64_NEXT_PASS_FRONTIER.md) through checked control flow; verify 37 reports / 632,236 bytes, thirteen changed / twenty-four retained.
- [x] Implement and pass targeted contracts for the [next-pass I4 object](docs/GX_MII_I4_32X64_NEXT_PASS_LOAD_2026-10-07.md), `0x80397F80`, slot 0, 32×64: 30 loads per mode, 2,980 / 3,002 diagnosed refusals.
- [x] Pass SDK/synthetic/private builds and lint on code `551cdbf`, retaining 71 strong functions and 48 scoped unique providers. NRO 73,621,560 bytes, SHA-256 `0b30ee17...`; 35 mutation checks, the 22-suite gate, final-head CI, merge and verified SD deployment are tracked in the development PR and issue #117.
- [x] Merge PR #329 (`19a2dd4`) after five final-head workflows / six jobs; verify complete SD readback and successful Netloader transfer at 16:48:04 CEST.
- [x] Verify [37 reports / 631,804 bytes](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_RELOCATED_DATA_FRONTIER.md), fourteen changed / twenty-three retained. The first object's data is `0x109C1A20`; this run stops before the next-pass object, whose return remains unconfirmed.
- [x] Implement and contract-test the [exact relocated first I4 tuple](docs/GX_MII_I4_RELOCATED_LOAD_2026-10-07.md): 35 loads per mode, 3,255 / 3,279 refusals, current-source refresh on the same native identity.
- [x] Reject all 40 compiled mutations and pass all 22 local suites; five final-head workflows / six actual jobs pass on `873495e`. Merge [PR #330](https://github.com/yashin-sh/WiiCompiled-Switch/pull/330) as `e72047d` and verify complete SD readback of NRO `a8718895...` at 17:24:56 CEST.
- [x] Establish [first relocated-source load and helper return](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_RELOCATED_DATA_FRONTIER.md) through the pinned unconditional caller and fresh second-object stop; verify 37 reports / 632,567 bytes, seven changed / thirty retained.
- [x] Implement and contract-test the [second relocated I4 object](docs/GX_MII_I4_SECOND_RELOCATED_LOAD_2026-10-07.md), slot 0, 32×64, data `0x109C1A20`: 40 loads per mode, 3,527 / 3,553 refusals, stable independent first/second native objects across source changes.
- [x] Reject all 42 compiled mutations and pass all 22 local suites; five final-head workflows / six actual jobs pass on `f8b8283`. Merge [PR #331](https://github.com/yashin-sh/WiiCompiled-Switch/pull/331) as `265839d`.
- [ ] Copy exact NRO `af9575be...` to SD and verify its complete byte/SHA-256 readback; the Switch's USB/MTP connection is currently unavailable.
- [ ] Establish second relocated-source and next-pass object return on Switch; GPU completion and recognizable images remain open.
- [ ] Resolve subsequent observed calls and establish recognizable game pixels.

- [x] Complete the offline CI/scripts/runtime/documentation audit and pass the local workflow gates plus private rendered build; see [the audit record](docs/PORT_AUDIT_2026-10-03.md). Its separate console run accepts normal-path non-regression only.

- [x] Hardware-cross GXSetCoPlanar, GXSetClipMode, indirect texture matrix/scale and ambient channel color on the documented Discovery path.
- [x] Hardware-cross the exact IA8 descriptor loads on maps 0..7.
- [x] Hardware-cross the ten type-0 GXLoadTexMtxImm loop calls (IDs 30,33,...,57), followed by return from Gen2 coord 0.
- [x] Capture the preceding DIRECT frontier: GXSetTexCoordScaleManually `0x80171180`, `(0,0,0,0)`, dispatch 605350, 99.513 seconds from the first dispatch.
- [x] Build and locally validate bounded coordinate candidate `91a4a01b8e316f9010e9d31754e279065772f9f3`; transfer its exact Rendered Discovery NRO with nxlink exit 0.
- [x] Retrieve fresh reports for NRO SHA-256 `64ba837720f4e37cbd127c37a0e9bde6dc146ed229a92c8697b9c531a8984d08`, transferred at 2026-10-02 23:09:38 UTC; the user confirms a black screen.
- [x] Hardware-accept the eight Gen2(c,1,4,60,0,125) / disabled Scale(c,0,0,0) / disabled Bias(c,0,0) triples on coords 0..7 by return to caller `0x80241380` and a later distinct frontier.
- [x] Record the new DIRECT frontier: GXSetTevDirect `0x80171B58`, stage 0, LR `0x80240F98`, dispatch 605633, 100.205 seconds from the first dispatch.
- [x] Launch audit NRO SHA-256 `7ecbc8a9fe1efb31697c2ee36d0b0b648a8e87d3fa7dda1fb9d262d6de5b7d09`, transfer exit 0 at 2026-10-03 09:25:19 UTC, and accept normal-path non-regression through TEV Direct stage 0 at dispatch 608381 / 107.925 seconds. Error-path fixes remain outside this hardware acceptance.
- [x] Pass the six-setter [TEV scalar candidate](docs/GX_TEV_SCALAR_BATCH_2026-10-03.md) local workflow/host-contract gates and exact private Rendered Discovery build.
- [x] Pass all five GitHub workflows on integrated code `e76e8f38`; transfer NRO `cc88a78c...` with exit 0 and hardware-accept the six TEV setters on default tuples across stages 0..15 (96 new calls plus 16 existing Order calls).
- [x] Capture the [KColor frontier](docs/HARDWARE_RESULTS_2026-10-03_TEV_SCALAR_KCOLOR_FRONTIER.md): ID 0, pointer `0x80398FCC`, dispatch 605056 / 98.265 seconds; user reports black screen and error at exit.
- [x] Implement the [bounded TEV color/table candidate](docs/GX_TEV_COLOR_BATCH_2026-10-03.md): KColor and adjacent Color/SwapModeTable, with ID-before-memory guards and passing executable host contracts.
- [x] Pass all five GitHub workflows and the exact private Rendered Discovery build for color/table code `1333b0e2`, NRO SHA-256 `a56be88113ff7c2cc20808111cf7d6c0e947b0c8955b2252737b974b28a9e0ad`, with 25 retained symbols and the three unique Aurora providers checked.
- [x] Transfer the color/table NRO with exit 0 at 13:43:58 UTC and establish fresh console return from all twelve calls through the restored later caller.
- [x] Capture [AlphaCompare](docs/HARDWARE_RESULTS_2026-10-03_TEV_COLOR_ALPHA_COMPARE_FRONTIER.md) `0x80172088`, (7,0,0,7,0), dispatch 700091, stage BlendMode; user still reports black output and a crash.
- [x] Implement the [separate AlphaCompare candidate](docs/GX_ALPHA_COMPARE_2026-10-03.md), including the existing host validity flag; pass both host-contract modes and four mutation checks.
- [x] Pass AlphaCompare rendered syntax, all five exact-code GitHub workflows and the private NRO build: SHA-256 `7032c756f4f0872334aea0a4421a8633e8d76d9ed1c004cf2d59fafa87b5b310`, 27 strong symbols, unique native and existing flag providers.
- [x] Transfer the exact AlphaCompare NRO with exit 0 at 17:50:16 UTC and accept the observed tuple return through existing ZMode to [Fog](docs/HARDWARE_RESULTS_2026-10-03_ALPHA_COMPARE_FOG_FRONTIER.md).
- [x] Capture Fog type 0, pointer `0x80398FD0`, four exact f64 parameter bits and readable RGBA 255,255,255,255 at dispatch 603961 / 99.156 seconds.
- [x] Implement the [bounded Fog/ZCompLoc candidate](docs/GX_FOG_Z_COMP_2026-10-03.md), including exact f64 guards, complete color range and full-word bool semantics; pass host/native contracts and rendered syntax.
- [x] Pass all five exact-bridge-code workflows and the private Fog/ZCompLoc NRO build, with 31 symbols and unique scoped native providers.
- [x] Validate the host-test-only core-dump optimization on all five workflows; preserve real SIGABRT and sanitizers, with byte-identical NRO. The observed host job is 10 min 39 sec versus 19 min 03 sec initially.
- [x] Transfer Fog/ZCompLoc NRO `652afed4...` with exit 0 at 19:20:07 UTC; accept both observed returns and the existing pixel setup from fresh coherent reports.
- [x] Capture the 4×4 `GX_TF_Z24X8` depth texture: native init passes, LOD rejects full format 22 at dispatch 609384 / 109.572 seconds.
- [x] Correct and locally contract-test the depth-texture LOD structural validation; see [the candidate](docs/GX_DEPTH_LOD_2026-10-03.md).
- [x] Pass all five workflows / six jobs on depth-LOD code `b5f0a2b0`, with actual new contract/native-fixture logs; build NRO `596ba38a...` with 35 strong symbols and unique scoped native Init/LOD providers.
- [x] Transfer NRO `596ba38a...` with exit 0 at 20:49:11 UTC; accept observed depth LOD return from fresh `lod-pass` and coherent later execution.
- [x] Capture GXBeginDisplayList `0x80172E00`, buffer `0x80394F00`, 16 KiB, dispatch 609010 / 108.440 seconds; user confirms black screen then error.
- [x] Hardware-accept Begin and at least one End return on the SU-corrected path, with later allocation and a new Sphere frontier; broader recording/replay validation remains open.
- [ ] Visually confirm a recognizable Mario Kart Wii image; no such image is established by the existing present counters.

The [latest console result](docs/HARDWARE_RESULTS_2026-10-07_MII_I4_SECOND_RELOCATED_DATA_FRONTIER.md) establishes first
relocated-source I4 load and intervening draw-helper return, then stops at the
second object `0x80397DC0`, slot 0, 32×64, data `0x109C1A20`, at
122.796 seconds. All 37 reports / 632,567 bytes are verified, seven changed /
thirty retained. The correction adds only this captured second identity while
keeping source-specific identity guards and independent native cache objects.
Second relocated-source and next-pass return, GPU completion and game pixels
remain unconfirmed. Retained copy/display-list reports are not fresh image
or GPU evidence; controller/audio and sustained gameplay remain open.

The milestone checklists below retain earlier scope and history. Older pending texture-object tuples remain scheduler-dependent branches; they are not the latest accepted Discovery frontier.

## M0 — libnx bootstrap
- [x] Minimal AArch64 `.nro` target
- [x] Atmosphère/hbmenu launch loop
- [x] Joy-Con / controller input smoke test
- [x] SD-card filesystem smoke test
- [x] CI build with devkitA64

## M1 — WiiCompiled platform audit
- [x] Pin a known-good WiiCompiled commit
- [x] Inventory OS/platform abstractions used by Windows/Linux/macOS
- [x] Identify POSIX assumptions incompatible with Horizon/libnx
- [x] Produce portability/blocker matrix for AArch64 Switch target

## M2 — runtime bring-up and translated boot fast-track
- [x] HostContext / coroutine backend
- [x] monotonic clock + sleep/yield
- [x] aligned allocation and GuestFlat guest-memory reservation strategy
- [x] generated data initialization handoff
- [x] link and enter real WiiCompiled-translated code locally
- [x] generic translated indirect-dispatch bridge
- [x] durable host exception and unsupported-dispatch diagnostics
- [x] robust SD diagnostic path creation / fallback
- [x] headless local fast-track that avoids the unrelated PrintConsole/NV framebuffer path
- [x] early cache/timebase/interrupt/exception bootstrap HLE
- [x] EXI/SI early bootstrap and basic transaction HLE
- [x] hardware-cross `OSReport` (`0x801A25D0`)
- [x] hardware-cross `OSGetConsoleType` (`0x8019F33C`)
- [x] hardware-cross `OSGetResetCode` (`0x801A8A50`)
- [x] hardware-cross `DCZeroRange` (`0x801A16E4`), including invalid-range/null-pointer handling
- [x] hardware-cross `IPCCltInit` (`0x80193478`) with translated `IPCInit` (`0x80192F7C`) handoff
- [x] hardware-cross `__OSInitSTM` (`0x801AB848`) with guest SDA state and fake STM handles
- [x] hardware-cross `NANDInit` (`0x8019E18C`) with SD-backed NAND root and guest home/init state
- [x] hardware-cross `NANDPrivateOpenAsync` (`0x8019C990`) far enough to reach `SCCheckStatus`; SD-backed open state and guest completion ABI are active
- [x] hardware-cross `SCCheckStatus` (`0x801B0220`) far enough to reach `DVDInit`; pinned immediate `SC_STATUS_OK` semantics are active
- [x] hardware-cross `DVDInit` (`0x8015EA1C`) far enough to reach `DVDLowClearCoverInterrupt`; startup-visible guest bookkeeping is active without fabricating FST data
- [x] hardware-cross `DVDLowClearCoverInterrupt` (`0x80166964`) far enough to reach `DVDLowInquiry`; pinned immediate-success semantics are active
- [x] hardware-cross `DVDLowInquiry` (`0x80165A30`) far enough to reach `ESP_InitLib`; command-block completion and shared DVD cancel/reset bookkeeping are active
- [x] hardware-cross `ESP_InitLib` (`0x801671D0`) far enough to reach `ESP_CloseLib`; pinned immediate-success semantics are active with no host `/dev/es` dependency
- [x] hardware-cross `ESP_CloseLib` (`0x80167224`) far enough to reach `NANDOpenAsync`; pinned no-op close and immediate-success semantics are active
- [x] hardware-cross `NANDOpenAsync` (`0x8019C918`) far enough to reach `NANDReadAsync`; SD-backed open state and guest completion ABI are active
- [x] hardware-cross `NANDReadAsync` (`0x8019B80C`) far enough to reach `NANDCloseAsync`; raw byte-count callback semantics and OK-zero async return are active
- [x] hardware-cross `NANDCloseAsync` (`0x8019CAEC`) far enough to reach PAL `main`; persistent-fd close state, closed guest openFlag and verbatim async return semantics are active
- [x] reach PAL Mario Kart Wii `main` (`0x8000B6B0`) on real Switch hardware after 605 translated dispatches
- [x] prove post-main execution enters `System::RKSystem::main` (`0x80008EF0`) and `System::RKSystem::initialize` (`0x80009194`)
- [x] identify and fix the first post-`main` runtime blocker under #117: missing Wii boot low-memory/MEM2 arena seed before the second `OSInitAlloc` (`0x801A0FC8`)
- [x] hardware-cross the repaired MEM2 allocation path far enough to reach `OSLockMutex` (`0x801A7EE4`)
- [x] hardware-cross `OSLockMutex` far enough to reach `OSGetCurrentThread` (`0x801A98B0`)
- [x] hardware-cross `OSGetCurrentThread` far enough to reach PAL `GXInit` (`0x8016B850`)
- [x] hardware-cross the PAL `GXInit` bridge far enough to reach PAL `VIInit` (`0x801B94A4`)
- [x] hardware-cross the PAL `VIInit` / `__VIInit` bridge far enough to reach `SCGetEuRgb60Mode` (`0x801B1CAC`)
- [x] hardware-cross the pinned PAL60 `SCGetEuRgb60Mode` bridge far enough to reach `SCGetAspectRatio` (`0x801B1BE4`)
- [x] hardware-cross the pinned-default widescreen `SCGetAspectRatio` bridge far enough to reach `VIGetDTVStatus` (`0x801BAD38`)
- [x] hardware-cross the pinned `VIGetDTVStatus` disabled/not-ready bridge far enough to reach `VISetBlack` (`0x801BAB2C`)
- [x] hardware-cross the pinned `VISetBlack` pending-state bridge far enough to reach `VIConfigure` (`0x801B9F6C`)
- [x] hardware-cross the pinned `VIConfigure` pending-state bridge far enough to reach `VIFlush` (`0x801BA9A4`)
- [x] hardware-cross the pinned `VIFlush` pending-state arm far enough to reach `GXSetDispCopySrc` (`0x8016F438`)
- [x] hardware-cross the pinned `GXSetDispCopySrc` state/FIFO bridge far enough to reach `GXSetDispCopyDst` (`0x8016F4B8`)
- [x] hardware-cross the pinned `GXSetDispCopyDst` state/FIFO bridge far enough to reach `VIWaitForRetrace` (`0x801B99EC`)
- [x] hardware-cross the pinned non-fiber `VIWaitForRetrace` retrace/commit bridge far enough to reach `VISetPostRetraceCallback` (`0x801B9138`)
- [x] hardware-cross the pinned `VISetPostRetraceCallback` registration bridge far enough to reach `OSCreateThread` (`0x801A9E84`)
- [x] hardware-cross the pinned guest-visible `OSCreateThread` bridge far enough to reach `OS__InitMessageQueue` (`0x801A72FC`)
- [x] hardware-cross the pinned guest-visible `OS__InitMessageQueue` bridge far enough to reach `OSResumeThread` (`0x801AA58C`)
- [x] hardware-cross the pinned `OSResumeThread` guest run-queue/scheduler handoff far enough to reach `SelectThread` (`0x801A9C08`)
- [x] hardware-cross the pinned guest scheduler `SelectThread` bridge far enough to select guest context `0x8042A680` and reach `OSLoadContext` (`0x801A1F58`)
- [x] hardware-cross the pinned non-fiber `OSLoadContext` restore/jump bridge into `EGG::Thread::start` (`0x8024373C`) far enough to reach `OSReceiveMessage` (`0x801A7424`)
- [x] hardware-cross the pinned `OSReceiveMessage` empty blocking-receive path far enough to reach `OSSleepThread` (`0x801AA9B8`) on receive wait queue `0x804294F8`
- [x] hardware-cross the pinned `OSSleepThread` wait-queue park / `SelectThread(0)` handoff far enough to expose saved SRR0 `0x80238A78`, an interior continuation inside `EGG::ProcessMeter::__ct` rather than a translated function entry
- [x] hardware-validate the HostContext-backed guest `OSThread` switch: the original translated host stack resumes past the `0x80238A78` interior continuation and returns from `HostContext::Switch`, exposing PAL `WPADInit` (`0x801BF5C4`) as the next direct blocker
- [x] hardware-cross the pinned PAL `WPADInit` contract initialization far enough to reach `WPADGetDpdSensitivity` (`0x801C329C`)
- [x] hardware-cross the pinned PAL `WPADGetDpdSensitivity` default sensitivity getter far enough to reach `WPADGetStatus` (`0x801BF64C`)
- [x] hardware-cross the pinned PAL `WPADGetStatus` contract-state getter far enough to reach `WPADControlMotor` (`0x801C0EC4`)
- [x] hardware-cross the pinned PAL `WPADControlMotor` no-op boundary far enough to reach `PADInit` (`0x801AF2F0`)
- [x] hardware-cross the pinned PAL `PADInit` idempotent host initialization far enough to reach `OSGetTime` (`0x801AAD5C`)
- [x] hardware-cross the pinned PAL `OSGetTime` rollover-safe time-base getter far enough to reach `OSSetPowerCallback` (`0x801AB75C`)
- [x] hardware-cross the pinned PAL `OSSetPowerCallback` SDA/STM bookkeeping bridge far enough to reach `SCGetProductArea` (`0x801B23A0`)
- [x] hardware-cross the pinned PAL `SCGetProductArea` SDK-table lookup far enough to reach `OSWakeupThread` (`0x801AAAA4`)
- [x] hardware-cross the pinned PAL `OSWakeupThread` wait-queue/run-queue/HostContext handoff into sustained post-main translated execution
- [x] prove a sustained run of 37,148 translated dispatches total / 36,543 post-main without a new unsupported-dispatch abort
- [x] extend sustained execution to 126,563 total / 125,958 post-main translated dispatches
- [x] map `0x8020FCD4` exactly to RMCP01 `PostRetraceCallback` and `0x8024373C` to `EGG::Thread::start(void*)`
- [x] observe callback `r3 = 0x365E`, proving guest VI retrace value advanced to 13,918
- [x] add an independent Horizon liveness watchdog that records ACTIVE vs STALE translated progress without mutating guest state
- [x] classify the sustained black-screen path as an active translated/VI display loop rather than a durable translated-thread stall
- [x] hardware-prove local RMCP01 FST publication at `0x97DC0000` with 64,224 bytes / 2,096 entries (#183)
- [x] disprove `DVDReadPrio` / `DVDReadAsyncPrio` as the earlier startup frontier: #185 was not reached in that first hardware run
- [x] hardware-prove the local RMCP01 DVD/FST path with a real `/Boot/Strap/eu/English.szs` read-pass (`299969` bytes)
- [x] isolate the durable priority-6 execution to later OSThread `0x90112660`, distinct from the initial `EGG::ProcessMeter` thread, and add lifecycle/vtable telemetry (#186)
- [x] identify its virtual `run()` as `0x80008D18` with object `0x8042E930` / vtable `0x80270BC0`, and correlate the starvation with the stale non-fiber `VIWaitForRetrace` path
- [ ] hardware-validate fiber-aware `VIWaitForRetrace`: priority-6 waiter must park on VI queue `0x80386BC0` and allow the default thread to resume between retraces
- [x] publish the user-owned RMCP01 FST into guest MEM2 and install the narrow local `DATA/files` DVD read mapping (#183/#185); hardware now proves both FST publication and a real `/Boot/Strap/eu/English.szs` read-pass
- [ ] move NAND async completion draining from the fast-track HLE boundary to a verified alarm/IOS scheduling point if later hardware ordering requires it
- [ ] complete thread/mutex/condition-variable semantics required by the game
- [ ] complete filesystem/NAND/DVD abstractions required by boot
- [ ] replace remaining temporary runtime/HLE stubs with verified semantics

Hardware evidence is recorded in:

- `docs/HARDWARE_RESULTS_2026-09-10.md`
- `docs/HARDWARE_RESULTS_2026-09-12.md`
- `docs/HARDWARE_RESULTS_2026-09-13_MAIN_REACHED.md`
- `docs/HARDWARE_RESULTS_2026-09-14_POST_MAIN_ACTIVE.md`
- `docs/HARDWARE_RESULTS_2026-09-15_OS_CREATE_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_INIT_MESSAGE_QUEUE.md`
- `docs/HARDWARE_RESULTS_2026-09-16_SELECT_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_LOAD_CONTEXT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_RECEIVE_MESSAGE.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_SLEEP_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-16_GUEST_FIBER_CONTINUATION.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_INIT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_DPD_SENSITIVITY.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_GET_STATUS.md`
- `docs/HARDWARE_RESULTS_2026-09-16_WPAD_CONTROL_MOTOR.md`
- `docs/HARDWARE_RESULTS_2026-09-16_PAD_INIT.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_GET_TIME.md`
- `docs/HARDWARE_RESULTS_2026-09-16_OS_SET_POWER_CALLBACK.md`
- `docs/HARDWARE_RESULTS_2026-09-16_SC_GET_PRODUCT_AREA.md`
- `docs/HARDWARE_RESULTS_2026-09-17_OS_WAKEUP_THREAD.md`
- `docs/HARDWARE_RESULTS_2026-09-17_SUSTAINED_LIVENESS.md`
- `docs/HARDWARE_RESULTS_2026-09-18_ACTIVE_RETRACE_LOOP.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_VULKAN_CLEAR_FRAME.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_AURORA_GX.md`
- `docs/HARDWARE_RESULTS_2026-09-18_M3_HLE_FIFO_AURORA.md`
- `docs/HARDWARE_RESULTS_2026-09-19_RMCP01_RESOURCE_THREAD_FRONTIER.md`

The current hardware-driven method remains deliberate after `main`: execute the broadest safe translated path, stop on the first unsupported native/translated boundary or attributable exception, and when no blocker appears use the independent heartbeat watchdog to distinguish sustained execution from a real stall. The default remains hardware evidence plus pinned WiiCompiled semantics; the user-authorized exception is a documented bounded GX batch with wrapper/Aurora audits and executable contracts. Each member still needs its own hardware progression proof. Post-main bring-up remains tracked in #117.

Validation policy after the 2026-09-20 audit: the five public CI workflows remain
mandatory under the project validation policy. The main-branch rules do not yet
require their job contexts; see [the audit](docs/PORT_AUDIT_2026-10-03.md).
Rendered RMCP01 changes additionally require a successful private
`build-local-rendered-fast-track.sh` build before hardware testing. A dispatch
hit counter is telemetry, not a standalone PASS; a boundary is hardware-crossed
only when execution durably progresses beyond the tested target. See
`docs/FAST_TRACK_VALIDATION_POLICY.md`.

## M3 — graphics / first frame
- [ ] Resolve shared upstream GX safety blockers before attributing failures to a Switch backend:
  - [ ] #109 — release-safe FIFO bounds checking
  - [ ] #110 — prevent draw merges across different `GXVtxFmt` values
  - [ ] #111 — guard `GX_LINESTRIP` zero/short vertex counts
  - [ ] #112 — make unsupported indexed XF loads visible in Release builds
- [x] Hardware-unblock M3 by proving the current black-screen runtime remains active through 13,918 VI retraces
- [x] Hardware-present an isolated loaderless NVK / `VK_NN_vi_surface` clear frame on real Switch (#162/#165)
- [x] Present a simple Vulkan triangle on the proven VI/NVK swapchain — **hardware PASS: visible triangle on real Switch (2026-09-18); SD report bug fixed separately**
- [x] Prove Dawn/WebGPU over the proven Vulkan/NVK path — **hardware PASS: 1280x720 surface, first present, 1,507-frame stable loop**
- [x] Prove a WGSL shader + Dawn graphics pipeline + triangle on real Switch — **hardware PASS: visible RGB triangle + clean explicit teardown**
- [x] Select the native Switch graphics strategy compatible with WiiCompiled/Aurora — **Aurora GX → Dawn/WebGPU → Vulkan/NVK is the primary path; Deko3D remains fallback**
- [x] Present an isolated Aurora GX triangle on real Switch — **hardware PASS: first Aurora GX triangle + 563-frame active loop + clean teardown**
- [x] Prove pinned WiiCompiled `HleFifoWrite` → Aurora GX with a fabricated Nintendo-data-free FIFO stream — **hardware PASS: exact pin, raw-direct path, 1,435-frame stable loop**
- [x] Prove a real GX → Switch command/backend path in the separate rendered target while retaining the headless sink as a control — **real RMCP01 FIFO work + first game-facing GPU present hardware-proven**
- [x] Render first native Switch clear frame
- [x] Reach first real RMCP01 GPU present through pinned FIFO → Aurora → Dawn/NVK
- [ ] Visually confirm the first Mario Kart Wii image and continue GX/game-state correctness
- [x] Validate a Nintendo-data-free desktop Aurora capture/replay with resource relocation, ordered updates and independent-process PNG comparison — **local lavapipe PASS, red/blue/background pixel oracles and byte-identical PNGs**; [prototype scope](docs/DESKTOP_GX_REPLAY.md)
- [x] Add opt-in first-frame Switch capture with GXInit, Aurora FIFO/direct-list consumption, native EFB copies, checked guest resources and explicit invalidation; validate synthetic copies/indexed arrays on desktop — [scope and build](docs/DESKTOP_GX_REPLAY.md)
- [x] Collect a valid RMCP01 Switch first-frame capture and validate its desktop replay output — **complete stream, one untextured quad, reproducible black PNG; game-image correctness remains open**; [hardware result](docs/HARDWARE_FIRST_FRAME_REPLAY_2026-10-08.md)
- [ ] Extend replay to later-frame checkpoints, cross-frame EFB resources, resource aliases/growth and remaining commands
- [ ] shader/pipeline cache strategy
- [ ] 720p handheld / 1080p docked policy

The upstream GX audit behind issues #109–#112 is recorded in `docs/UPSTREAM_GX_AUDIT_2026-09-13.md`. These are shared decoder/runtime concerns and must be separated from Switch-backend-specific failures during M3.

> The stable #117 fast-track intentionally keeps its FIFO sink as a control
> baseline. The rendered path is now hardware-proven through real
> English.szs/StaticR/Home Button resource loading, real FIFO work, repeated
> GXCopyDisp/present, eleven hardware-crossed GXInitTexObjLOD descriptors plus ninth/twelfth captured exact descriptors,
> seven hardware-crossed GXInitTexObjWrapMode tuples plus sixth/eighth/tenth/eleventh captured exact tuples on obj 0x908FA5C0 / 0x909019C0 / 0x9018E480 / 0x908FAE00,
> and the exact KD sequence through fd 2003 close.
> AIInit (0x801240B0), __AXOutInitDSP (0x801269BC), AIRegisterDMACallback (0x80123F88), AIInitDMA (0x80123FCC), and AIStartDMA (0x80124048) are hardware-crossed.
> The third wrap tuple on obj 0x9018E140 and fourth LOD tuple on obj 0x9018E480
> are hardware-crossed. OSSetPeriodicAlarm (0x801A08E0) is also hardware-crossed.
> Earlier scheduler-dependent texture-object, KD and audio gates are retained
> in the dated hardware reports. The current accepted Discovery frontier and
> the coordinate candidate awaiting reports are listed in the checkpoint above.
> A recognizable Mario Kart Wii image remains unproven.

## M4 — input + audio
- [ ] Map Joy-Con / Pro Controller to WiiCompiled input
- [ ] Rumble
- [ ] Audio output
- [ ] latency instrumentation

## M5 — first game boot / playable offline path
- [x] User-owned local translation/build pipeline exists
- [x] enter translated Mario Kart Wii startup on real Switch hardware
- [x] reach game `main()` on real Switch hardware
- [x] capture and fix the first post-main blocker (#117)
- [x] hardware-cross the observed post-main scheduler/input/time/SC sequence through `OSWakeupThread`
- [x] reach sustained translated execution in the EGG display subsystem
- [x] classify the sustained black-screen path as an active VI/post-retrace loop
- [x] attribute the #188 first-fiber crash at `0x8042A680 -> 0x8024373C` to runtime-boundary VI polling clobbering the interrupted `CpuContext`
- [x] hardware-validate register-isolated VI polling and restore the first guest-fiber blocking path
- [x] hardware-validate the `0x90112660` fiber-aware `VIWaitForRetrace` starvation fix: worker parks on `0x80386BC0`, scheduler returns to main
- [x] hardware-cross PAL `OSSendMessage` (`0x801A735C`) after scheduler recovery
- [x] hardware-cross PAL `GXDrawDone` (`0x8016EAB0`) using the pinned draw-done bookkeeping and Aurora FIFO drain
- [x] hardware-cross virtual `EGG::TaskThread::run` (`0x80242D7C`) with the explicit hit counter
- [x] classify the 2026-09-22 TaskThread indirect-dispatch anomaly: received `job=0x8042E448` aliases the worker stack and decodes to `callback=0x8042E458`, `onDone=0x801AA0F0`
- [x] prove the translated producer path is correct: `TaskThread::request` sends `mJobs[0]=0x8042E7DC` into the matching queue/buffer
- [x] identify the clobber phase: `after-output-write` is `0x8042E7DC`, then `after-wakeup-senders` is `0x8042E448`
- [x] attribute the empty-waiter wakeup boundary to the Switch pre-call VI poll while guest interrupts are disabled
- [x] hardware-validate that masking VI polling during disabled guest interrupts preserves `0x8042E7DC` across wakeup/interrupt restore and advances to the real callback `0x8000B53C`
- [x] hardware-cross the real TaskThread callback far enough to read `/Boot/Strap/eu/English.szs` successfully through the local DVD bridge
- [x] attribute `SELECTTHREAD_IDLE_POLL`: default thread `0x80347498` parks on `0x804294A4` from `LR=0x8020FE50`; queue is `AsyncDisplay + 0x58` and EGG `postVRetrace()` is the matching wake source
- [x] hardware-validate the VI-only SelectThread idle wake: AsyncDisplay/default thread leaves `0x804294A4`, becomes READY, resumes, and recovers real FIFO/present work
- [x] hardware-cross pinned `EGG::Decomp::decodeSZS (0x80218C2C)`: `English.szs` consumes 299,969 compressed bytes and produces 2,627,200 decompressed bytes
- [x] hardware-cross pinned `GXInitTexObj (0x801707F8)` using the live texture format/wrap/mipmap registers without pre-porting `GXLoadTexObj`, CI, LOD or TLUT neighbors
- [x] attribute pinned `NAND_IOS_Open (0x801938F8)`: hardware path is `/dev/net/kd/request`, mode 0
- [x] merge exact `/dev/net/kd/request`, mode 0, device-allocation bridge as PR #232 after 5/5 public CI
- [x] hardware-cross only the pinned KD-request device allocation (first fd 2000); do not pre-port neighboring network devices
- [x] capture live `r7/r8` for pinned `IOS_Ioctl (0x80194290)`: outBuf `0x80356F40`, outLen `0x20`, with fd 2000 / KD command 2
- [x] implement only the first Boot-phase KD command-2 reply: write WC24 `-42` to the live output word and return IOS result `0`; repeated command 2 and command 1/3 remain unsupported
- [x] hardware-cross the merged first KD command-2 probe: status `cmd2-boot-probe-pass` followed by durable progress to `IOS_Close (0x80193AD8)`
- [x] attribute `0x80193AD8` to pinned `NAND_IOS_Close_HLE(fd)`; hardware passes fd 2000, the proven KD request handle
- [x] implement only the exact fd-2000 network-device close: retire the local first-KD handle and return IOS result `0`; other closes remain unsupported
- [x] hardware-cross the merged fd-2000 IOS_Close: `close-pass` followed by durable progression into RKSystem::run / StaticR resource loading
- [x] hardware-validate local DVD read of `/rel/StaticR.rel`: 4,903,876 bytes
- [x] attribute the next exact blocker `0x80170F2C` to pinned `GXLoadTexObj(oa, tid)`; live oa=`0x901136B4`, tid=0
- [x] capture the exact 32-byte guest GXTexObj at `0x901136B4`: words `90/0/471f3f/7881f/0/4/0/5ca00202`, decoded as 832x456 format 4, clamp/clamp, no mipmaps, data `0x00F103E0`
- [x] implement only that exact first `GXLoadTexObj`: validate all eight words, mapped 0xB9400-byte payload, reconstruct Aurora GXTexObj, apply decoded linear/linear LOD state, bind map 0, and mirror pinned GXData dirty state
- [x] hardware-cross the exact first GXLoadTexObj: load-pass followed by durable progression to a distinct DIRECT blocker
- [x] attribute 0x8016E37C to pinned GXSetTexCoordGen2(dc,type,src,mtx,normalize,postMtx)
- [x] implement only the observed tuple: TEXCOORD0 / MTX2x4 / TEX0 / IDENTITY / false / PTIDENTITY
- [x] hardware-cross the exact GXSetTexCoordGen2: later run reaches 19,718 translated dispatches, 60 GXFlush/GXCopyDisp and 60 successful presents
- [x] attribute 0x800077C8 to pinned StrapScene__CheckInput_Skip(scenePtr)
- [x] implement only the observed scenePtr 0x90112A34 and pinned guest-visible return r3=1; omit desktop settings-overlay notification
- [x] hardware-cross the exact strap input acceptance: later durable execution reaches 17,800 dispatches, 765 FIFO writes and 61 successful presents / 0 failures
- [x] attribute `INDIRECT_CALL_MISS 0x8055531C` to pinned StaticR.rel `RelProlog`; observed module base `r3=0x805102E0`
- [x] confirm the pinned runtime registers `StaticRProlog_RecompModInit_8055531c` as a native winner wrapping the retained translated `func_8055531C`
- [x] implement only that exact native seam: guard the observed StaticR base and execute the already-generated original RelProlog; no speculative REL loader/relocation/RelEpilog behavior
- [x] hardware-cross the exact StaticR RelProlog candidate: 269 StaticR dispatches plus durable later DOL/OS execution
- [x] attribute the next exact DIRECT blocker `0x801AA4EC` to pinned `OSDetachThread`; live r3=`0x901187C0` TaskThread
- [x] add diagnostics-only capture for the exact OSThread state/attributes/join queue/list links required to choose the pinned detach branch
- [x] capture the exact OSDetachThread live state: WAITING state 4, attr=1, empty join queue, known guest fiber
- [x] implement only that observed non-MORIBUND OSDetachThread path
- [x] hardware-cross the exact OSDetachThread candidate and identify the next blocker
- [x] attribute the next exact DIRECT blocker `0x801AA1D4` to pinned `OSCancelThread`
- [x] add diagnostics-only capture for wait queue, mutex ownership, global list, scheduler flags and fiber/current-context state
- [x] capture the exact OSCancelThread live state: WAITING, detached, singleton wait queue, no joiners/mutexes, non-current known fiber
- [x] implement only that observed OSCancelThread termination path
- [x] hardware-cross the exact OSCancelThread candidate and identify the next blocker
- [x] attribute the next exact DIRECT blocker `0x80170A4C` to pinned `GXInitTexObjLOD`
- [x] record Home Button/UI resource progress through HomeButton.arc, homeBtn_ENG.szs and related HBM assets
- [x] add diagnostics-only capture for f1/f2/f3 effective float values/bits and all eight GXTexObj words
- [x] capture the complete GXInitTexObjLOD tuple: min/mag=1/1, f1/f2/f3=+0.0f, bc/el/aniso=0/0/0, exact eight-word descriptor
- [x] implement only that exact GXInitTexObjLOD tuple and pinned guest word0/word1 update
- [ ] hardware-cross the exact GXInitTexObjLOD candidate and identify the next blocker
- [x] hardware-cross PAL `GXSetProjection` (`0x8017301C`) using the pinned guest-matrix -> Aurora contract
- [x] hardware-cross PAL `GXSetViewport` (`0x801733B4`) using PPC f1..f6 and the pinned Aurora viewport contract
- [x] hardware-cross PAL `GXSetScissor` (`0x80173430`) using pinned guest GXData bookkeeping plus Aurora scissor
- [x] hardware-cross PAL `GXLoadPosMtxImm` (`0x8017310C`) using the pinned 3x4 guest-matrix -> Aurora contract
- [x] hardware-cross PAL `GXSetCurrentMtx` (`0x80173214`) using the pinned `r3` matrix-id -> Aurora contract
- [x] hardware-cross PAL `GXClearVtxDesc` (`0x8016DC34`) using pinned descriptor-state reset + Aurora contract
- [x] hardware-cross PAL `GXSetVtxDesc` (`0x8016D3A4`) using pinned HLE descriptor bookkeeping + Aurora direct-stream contract
- [x] hardware-cross PAL `GXSetVtxAttrFmt` (`0x8016DC68`) using pinned HLE format bookkeeping + Aurora contract
- [x] hardware-cross PAL `GXSetNumChans` (`0x8017054C`) using pinned `r3 -> u8 -> Aurora` contract
- [x] hardware-cross PAL `GXSetChanMatColor` (`0x80170474`) using pinned guest RGBA read/decode + Aurora contract
- [x] hardware-cross PAL `GXSetChanCtrl` (`0x80170570`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetNumTexGens` (`0x8016E5A4`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetNumIndStages` (`0x80171B38`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetNumTevStages` (`0x801722A8`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetTevOp` (`0x80171C4C`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetTevOrder` (`0x8017214C`) and progress to the next distinct durable DIRECT blocker
- [x] hardware-cross PAL `GXSetBlendMode` (`0x8017277C`) and progress to the next distinct durable DIRECT blocker
- [x] implement PAL `GXSetColorUpdate` (`0x801727CC`) with pinned `r3 -> GXBool -> Aurora` contract (#212)
- [x] hardware-cross merged `GXSetColorUpdate` and progress to the next exact hardware-observed boundary
- [x] implement and hardware-cross PAL `GXSetAlphaUpdate` (`0x801727F8`) with pinned `r3 -> GXBool -> Aurora` contract
- [x] implement and hardware-cross PAL `GXSetZMode` (`0x80172824`) with pinned live `r3/r4/r5 -> GXBool/GXCompare/GXBool -> Aurora` contract
- [x] implement and hardware-cross PAL `GXSetCullMode` (`0x8016F3B8`) with pinned `r3 -> GXCullMode -> Aurora` contract
- [x] implement and hardware-cross PAL `GXBegin` (`0x8016F0F0`) with pinned `r3/r4/r5 -> primitive/vtxfmt/count -> HleFifoWrite/Aurora begin-state` contract
- [x] prove first real RMCP01 drawable FIFO work — `PASS FIRST_RMCP01_FIFO_WORK`
- [x] implement and hardware-cross `EGG::AsyncDisplay::endRender` (`0x8020FF9C`) with pinned nested translated calls
- [x] implement and hardware-cross PAL `GXSetCopyFilter` (`0x8016FA40`) with pinned live `r3..r6` + guest 24-byte/7-byte filter data -> Aurora contract
- [x] prove game-facing `GXCopyDisp` / first GPU present — `PASS FIRST_RMCP01_GX_PRESENT hadWork=1`
- [x] implement and hardware-cross PAL `GXFlush` (`0x8016E654`) with pinned no-argument Aurora contract — 23 hits, 23 successful presents and durable later execution
- [ ] visually confirm the first Mario Kart Wii image
- [ ] continue post-main initialization through system/resource initialization
- [ ] complete game/resource initialization
- [ ] menus
- [ ] offline time trial
- [ ] Grand Prix / VS

## M6 — performance
- [ ] CPU profiling on Horizon
- [ ] remove costly synchronization
- [ ] reduce translated-state overhead
- [ ] graphics submission optimization
- [ ] frame pacing
- [ ] target stable 60 FPS where feasible

## M7 — online/mod compatibility
- [ ] save data
- [ ] RetroWFC compatibility
- [ ] Retro Rewind compatibility evaluation
