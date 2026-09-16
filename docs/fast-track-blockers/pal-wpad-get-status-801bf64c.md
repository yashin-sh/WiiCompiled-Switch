# PAL WPADGetStatus blocker — 0x801BF64C

Hardware after #148 reaches PAL `WPADGetStatus` (`0x801BF64C`) as a direct unsupported native boundary at `HOST_CONTEXT_SWITCH_RETURNED`.

Pinned WiiCompiled returns the shared WPAD library status only: `0` before initialization and `3` after `WPADInit`. The current hardware path already crossed the pinned `WPADInit` bridge, so this call returns `3`.

No controller/device behavior is required at this boundary.
