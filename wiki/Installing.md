# Installing

1. Close the game.
2. Extract the release zip into the game folder, next to `MBTL.exe`. It holds two files: `dinput8.dll` is the
   mod, and `MBTLIMUpdater.exe` installs later versions. The mod works without the updater.
3. Start the game normally.

The first launch creates `MBTL-IM\MBTL_IM.ini`, `MBTL-IM\Logs` and `MBTL-IM\Mods` if they are missing.

## Updating

When a newer release is out, the mod opens a window saying so. **Update now** downloads it, checks it against
the release checksum, closes the game, swaps the files and starts the game again through Steam. The previous
files are kept in `MBTL-IM\Updater\backups`, and `MBTL-IM\Updater\logs\updater.log` says what happened.

Turn the check off with **Check for updates on start** under Config, or `[Mod] CheckForUpdates = 0`.

## Linux and Proton

Add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the game's launch options, or rename `dinput8.dll` to
`d3d9.dll`. Keep only one of the two names in the folder.

The game's "Assertion failed" dialogs, which Proton cannot get past, never open with the mod installed. The mod
skips them the way **Ignore** does on Windows and writes each one to `MBTL-IM\Logs`.

## Uninstalling

Delete `dinput8.dll`. The `MBTL-IM` folder can stay; the game ignores it without the dll.
