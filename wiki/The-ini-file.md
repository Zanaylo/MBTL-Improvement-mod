# The ini file

`MBTL-IM\MBTL_IM.ini`. Missing keys are added with their defaults on launch.

| Section | Key | Default | What it does |
|---|---|---|---|
| `Debug` | `Logging` | `0` | Write a log to `MBTL-IM\Logs` |
| `ModFiles` | `Enabled` | `1` | Load files from `MBTL-IM\Mods` |
| `ModFiles` | `LogServedFiles` | `0` | Log every file served from Mods |
| `ModFiles` | `LogMissingFiles` | `0` | Log files the game asked for in a modded folder that Mods does not have |
| `Mod` | `DinputDllWrapper` | empty | Another `dinput8.dll` to chain to instead of the Windows one |
| `Mod` | `CheckForUpdates` | `1` | Ask GitHub once at start whether a newer release is out |
| `Compat` | `ShowGameAsserts` | `0` | Show the game's own "Assertion failed" dialogs. At `0` the mod skips them and writes each one to the log |
| `Netplay` | `SharePalettes` | `1` | Send your palette to an opponent who also runs the mod |
| `Netplay` | `HoldBackAddedStages` | `1` | Online, hide stages the game does not ship with unless the other side has the same ones |
| `Netplay` | `NetworkLog` | `0` | Write a connection log to `MBTL-IM\Logs` |
| `Palette` | `ShowOnlinePalettes` | `1` | Show the palette the other player sent |

Every log is off by default. Turn `Logging` on before reporting a bug, or `NetworkLog` on before
reporting a connection problem, then reproduce it and send the newest file from `MBTL-IM\Logs`.
