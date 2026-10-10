# Signed TEV material returned; DrawQuad boundary — 2026-10-10

The exact `d5f24b9` candidate from [PR #350](https://github.com/yashin-sh/WiiCompiled-Switch/pull/350)
transfers successfully at 23:02:39 UTC on October 9 (01:02:39 CEST on October
10). Its 74,432,568-byte NRO remains bound to SHA-256
`c554d17150415e34fd1976567d1c00568a5281c1904a81dc051f962f4e06ea62`.
After the user confirms the trial stopped and USB is ready, all 39 reports /
839,914 bytes match a second independent USB retrieval. Full SD NRO readback
also matches the candidate. Sixteen reports change; twenty-three are retained.

The fresh trace records material entry at dispatch 653057 and S10 ID 1 /
pointer `0x80398E80` at 653094, with the material's stack `0x80398E38`.
The checked private material caller executes the three S10 calls with IDs
1, 2 and 3 consecutively, without a branch between them. Dispatch 653116
then reaches the outer caller's layout helper with its restored stack
`0x80398F58` and LR `0x8007B294`. This accepts the three returns on the observed
material path by control-flow inference. It is not three individually logged
return events; the actual signed color components are not recorded.

The next durable stop is missing DIRECT target `0x80084D20`, identified as
`nw4r::lyt::detail::DrawQuad` by the pinned runtime's native-call registry.
It occurs at 58.737 seconds / dispatch 653128. Arguments are position
`0x80398F60`, size `0x92133DD8`, texture-coordinate count 1, coordinates
`0x92133E8C`, no color array and alpha 255. The stage is
`RMCP01_GX_SET_VTX_ATTR_FMT`. Checked caller setup establishes direct XY/F32
position and ST/F32 texture coordinates before this call. The
[new bridge](LYT_DRAW_QUAD_2026-10-10.md) validates the actual runtime layout
before emitting the quad.

The fresh graphics report contains no Dawn uncaptured errors; no exception
report is present. An earlier snapshot at dispatch 650229 records 103
successful presents and zero present failures, with coherent current/running
fiber state. Those counters precede the final boundary and are not new pixel
proof. Six presentation windows cover 102 frames / 52,746 ms, a weighted
1.934 Hz, with individual windows from 0.099 to 8.950 Hz. This is presentation
telemetry from an uncontrolled diagnostic run, not a performance comparison.

Both current-run capture status files are independently verified `DISABLED`,
with fresh run identifiers. Old PNGs and replay archives are not attributed to
this trial. The user reports that the trial stopped without a screen-specific
observation. Later game pixels, DrawQuad return on hardware and playability
remain unproven. Raw reports and generated caller sources remain private.
