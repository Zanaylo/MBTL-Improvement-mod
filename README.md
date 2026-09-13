# MBTL Improvement Mod

A mod for MELTY BLOOD: TYPE LUMINA with an in-game window, built on the same design as UNI2 Improvement Mod.

- **Mod loader** - the game reads files from `MBTL-IM\Mods` and from switchable mods in `MBTL-IM\Packs`
  instead of its own archives, without changing `MBTL.exe` or any `data*.bin`.
- **Hitbox viewer**, **Pause** and **Next frame** in offline matches.
- **Stages** - install stages from UNDER NIGHT IN-BIRTH II, UNI[st], UNI[cl-r], UNI Exe:Late and DENGEKI BUNKO
  FIGHTING CLIMAX IGNITION, or a stage folder of your own, unhide the stages the game hides, and an optional
  extension table of 127 stage numbers.
- **Music** - every track, your own OGG music, and rules that swap one track for another.
- **Performance** - frame pacing fixes and frame time metrics.
- **Config** and **Debug** windows.

## Install

1. Copy `dinput8.dll` and the `MBTL-IM` folder next to `MBTL.exe`.
2. Start the game and press F1.

On Linux/Proton, add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options, or rename
`dinput8.dll` to `d3d9.dll`.

## Build

```
MSBuild MBTL_IM.slnx /p:Configuration=Release /p:Platform=Win32
```

Win32 only. `/p:DeployToGame=false` keeps the dll out of the game folder, `/p:EnableLogging=true` forces the log.
