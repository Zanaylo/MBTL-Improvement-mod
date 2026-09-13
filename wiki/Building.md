# Building

Visual Studio 2026 with the C++ desktop workload. Win32 only.

| Property | Effect |
|---|---|
| `/p:DeployToGame=false` | Do not copy the dll into the game folder |
| `/p:GameDir=<path>\` | Deploy somewhere else |
| `/p:EnableLogging=true` | Always log, whatever the ini says |
| `/p:PlatformToolset=v143` | Build with Visual Studio 2022 |

The build copies `dinput8.dll` and its `.pdb` into the game folder unless `DeployToGame=false`. The game
locks the dll while it runs, so close it first.
