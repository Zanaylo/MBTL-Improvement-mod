# Replacing game files

Put a file under `MBTL-IM\Mods` at the same path it has inside the game's archives.

| File in Mods | What it does |
|---|---|
| `bg\BgList.txt` | the stage table |
| `bg\BgList_str.ini` | stage names |
| `bg\bg036\bg.fbx.bin` | stage 36's model |
| `bg\bg65535\stage_color.img` | character select lighting |
| `Bgm\bgm.txt` | the music table |

## Rules

- Files are plain, unencrypted files, the same thing an unpacker writes out.
- New files work, not only replacements, so new stages can be added.
- Two mods that change the same file, such as `bg\BgList.txt`, have to be merged by hand.

## The log

`MBTL-IM\Logs` holds one log per launch. It shows whether the loader found the game's file reader, every
file it served, and every file the game looked for in a modded folder but did not find. If a game update
moves things the loader turns itself off, the game runs unmodded, and the log says why.