# KPAD unified status returned; PADControlMotor frontier — 2026-10-05

The latest retrieved run crosses the count-1 KPADGetUnifiedWpadStatus polling
path and stops at **PADControlMotor `0x801AF908`**, channel 0, command 2,
dispatch **618177 / 109.316 seconds**. The user confirms **black output
followed by an error**. Recognizable game pixels remain unproven.

## Candidate and verified evidence

The Rendered Discovery NRO contains code
`aa73a0c51a42496a01b476a9921697ea53dde1da`. Launch revision `6a43ec8` changes
Markdown only. The private NRO is **73,494,584 bytes**, SHA-256
`fa05c8703c38ab2361e61562b8e8203e9fe99becfd3ea8510706a57435db0645`.
Its local hash was rechecked against the launch metadata. The direct nxlink
transfer exited 0 on **2026-10-04 21:42:13 UTC**, after sending 26,743,208
compressed bytes / 2,247 blocks. No TCP preflight was used.

USB/MTP retrieval on **2026-10-05 04:43:11 UTC** supplied **34 reports /
544,124 bytes**. Every copied size, SHA-256, difference from the preceding
WPADProbe baseline, raw ZIP member byte and CRC was independently verified.
Fifteen reports differ, including the new KPAD report; nineteen are identical.
Source timestamps are unavailable. Identical reports cannot independently
establish fresh calls. Private NROs, raw reports and diagnostic archives remain
excluded from publication.

## Accepted polling path

Discovery first observes KPADGetUnifiedWpadStatus `0x8019812C` at dispatch
617909, channel 0, output `0x80398F50`, requested count 1, LR `0x8051EEC0`,
stack `0x80398F38`. The new report ends with `no-remote-pass`, channel 3,
the same output pointer, one 56-byte entry written and valid-sample return
count 0. WPADProbe's report also ends on channel 3 with result -1.

The checked translated caller `0x805237E8` loops over four controller objects;
`0x8051ED14` invokes their input methods, and `0x8051FC84` calls WPADProbe
then KPAD with count 1 for valid channels. Discovery subsequently reaches
`0x805201B0` at dispatch 618145 with caller stack restored to `0x80398FA8`,
then `0x80524628` at 618164 and `0x80522840` at 618177. Together, the first
channel-0 entry, final channel-3 report and later caller progression establish
return through this polling path. Intermediate calls are inferred from the
checked loop, not individually recorded: discovery logs first hits only.
The raw 56-byte guest output is not captured. Larger counts, alternate
buffers, other input methods and physical button behavior retain host proof
or remain open; this run does not validate them on hardware.

## New exact stop

The durable DIRECT blocker records `0x801AF908`, r3=0 (channel), r4=2
(hard-stop motor command), r5=`0x9025EBE0`, r6=`0x9025AFB0`,
LR `0x8051EEEC`, stack `0x80398FA8`, r2=`0x8038EFA0`,
r13=`0x8038CC00`. The stage is `RMCP01_KPAD_UNIFIED_STATUS`, the last
completed native marker, rather than evidence of a stop inside KPAD.

The pinned WiiCompiled `runtime/src/hle/input/pad.cpp` identifies the address
as `PAD__ControlMotor_HLE` / PADControlMotor. Local doldecomp attribution
independently places it in `rvl/pad/rvlPad.c`. The executed caller
`0x80522840` reads its object's channel field and sets command 2 before this
direct call. This is a fresh unsupported boundary; motor control has not
returned and no rumble behavior is accepted.

## Runtime and rendering limits

The fresh heartbeat at dispatch 618041 retains coherent guest fiber / OS
current / running identities `0x80347498`, six TaskThread hits, structurally
valid FST, 3,937 FIFO writes, 168 GXBegin hits, 103 GXFlush hits,
99 successful presents / zero failures and zero display-list replay calls.
This snapshot precedes the final channel-3 report and blocker; it does not
establish visible content after KPAD. The watchdog has 105 ACTIVE samples
and one recovered STALE sample, maximum interval 1,262 ms and a final ACTIVE
sample. Dispatch/time differences are not performance comparisons.

PADControlMotor, broader replay, recognizable images, sustained execution,
full input/audio behavior and performance remain open. See the
[KPAD implementation and validation](KPAD_UNIFIED_STATUS_2026-10-04.md) and
[preceding WPADProbe result](HARDWARE_RESULTS_2026-10-04_WPAD_PROBE_KPAD_UNIFIED_FRONTIER.md).
