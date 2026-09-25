# PAL WPADSetSyncDeviceCallback blocker — `0x801BF640`

The 2026-09-25 rendered real-Switch run after main
`81d383bdd7dd2eda7dcab459a581026615000867` stops on direct target
`0x801BF640` at fast-track stage `HOST_CONTEXT_SWITCH_RETURNED`.

Observed argument:

```text
r3 / callback = 0x805230E0
```

Pinned WiiCompiled
(`patchzyy/Wiicompiled@a135beb201042b20f390c6695ca6b26768820fb4`)
maps this address exactly to `WPADSetSyncDeviceCallback_HLE`.

Pinned semantics are a pure host-side state swap:

```text
previous = syncDeviceCallback
syncDeviceCallback = callback
return previous
```

The pinned state starts at zero, so the first observed call returns `0` and
stores `0x805230E0`.

The Switch bridge must implement only this boundary. It must not invoke the
callback, start or stop simple sync, probe devices, or add controller mapping
ahead of hardware evidence.

Crossing proof requires a later durable frontier or milestone beyond
`0x801BF640`; a compile-only/synthetic probe is not hardware acceptance.
