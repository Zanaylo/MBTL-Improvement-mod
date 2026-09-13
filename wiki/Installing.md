# Installing

1. Close the game.
2. Copy `dinput8.dll` and the `MBTL-IM` folder into the game folder, next to `MBTL.exe`.
3. Start the game normally.

The first launch creates `MBTL-IM\MBTL_IM.ini`, `MBTL-IM\Logs` and `MBTL-IM\Mods` if they are missing.

## Linux and Proton

Add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the game's launch options, or rename `dinput8.dll` to
`d3d9.dll`. Keep only one of the two names in the folder.

## Uninstalling

Delete `dinput8.dll`. The `MBTL-IM` folder can stay; the game ignores it without the dll.
