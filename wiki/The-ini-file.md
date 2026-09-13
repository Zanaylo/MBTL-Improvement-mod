# The ini file

`MBTL-IM\MBTL_IM.ini`. Missing keys are added with their defaults on launch.

| Section | Key | Default | What it does |
|---|---|---|---|
| `Debug` | `Logging` | `1` | Write a log to `MBTL-IM\Logs` |
| `ModFiles` | `Enabled` | `1` | Load files from `MBTL-IM\Mods` |
| `ModFiles` | `LogServedFiles` | `1` | Log every file served from Mods |
| `ModFiles` | `LogMissingFiles` | `1` | Log files the game asked for in a modded folder that Mods does not have |
| `Mod` | `DinputDllWrapper` | empty | Another `dinput8.dll` to chain to instead of the Windows one |
| `Mod` | `CheckForUpdates` | `1` | Ask GitHub once at start whether a newer release is out |
