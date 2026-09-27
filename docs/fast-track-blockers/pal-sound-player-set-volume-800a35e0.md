# PAL nw4r::snd::SoundPlayer::SetVolume frontier — `0x800A35E0`

The 2026-09-27 rendered real-Switch run hardware-crosses
`OSSetPeriodicAlarm (0x801A08E0)`, loads real `/sound/revo_kart.brsar`
content, and reaches:

```text
target      = 0x800A35E0
soundPlayer = 0x90359920
stage       = RMCP01_GX_FLUSH
```

Pinned WiiCompiled attributes the target to
`nw4r::snd::SoundPlayer::SetVolume`. PPC r3 carries the SoundPlayer pointer
and scalar volume is carried in f1. The guest-visible operation clamps the
volume to the original NW4R range and writes it to `soundPlayer + 0x2C`.

The Switch candidate implements only that write. Desktop media attenuation and
all neighboring sound/DSP functions remain outside this blocker.
