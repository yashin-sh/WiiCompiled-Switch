# PAL WPADInit blocker — `0x801BF5C4`

The 2026-09-16 hardware run after #146 stopped on direct target `0x801BF5C4` at fast-track stage `HOST_CONTEXT_SWITCH_RETURNED`.

This address is `WPADInit` in pinned WiiCompiled (`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`). The pinned native override performs only `WpadContract::State::Initialize()` and returns `0`.

The Switch bridge intentionally mirrors only that observed boundary. It does **not** implement WPAD probing, synchronization, extension detection, callbacks, rumble, or real Joy-Con/Wii Remote mapping ahead of hardware evidence.

The hardware result also closes the previous `0x80238A78` interior-resume blocker: the HostContext-backed translated continuation now returns successfully far enough to expose `WPADInit` as the next unsupported native call.
