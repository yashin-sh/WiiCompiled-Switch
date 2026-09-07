# Legal boundaries

This is a homebrew interoperability/porting project. It does not distribute Mario Kart Wii or Nintendo assets.

## Never commit

- Wii disc images (`.iso`, `.wbfs`, `.rvz`, etc.)
- Nintendo keys, firmware, tickets, certificates, title keys or prod.keys
- extracted game textures, audio, models, executable sections or other copyrighted assets
- generated WiiCompiled output if it embeds copyrighted game data

## Local-only inputs

Any future game-data preparation step must consume a user's own dump locally and write generated output to ignored directories.

## Licensing

WiiCompiled is licensed under GPL-3.0. Any source copied or modified from WiiCompiled must retain notices and GPL-3.0 obligations.

Atmosphère is not bundled by this project. libnx is used as the public homebrew API boundary.
