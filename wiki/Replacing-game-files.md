# Replacing game files

Put a file under `MBTL-IM\Mods` at the same path it has inside the game's archives.

| File in Mods | What it does |
|---|---|
| `bg\BgList.txt` | the stage table |
| `bg\BgList_str.ini` | stage names |
| `bg\bg036\bg.fbx.bin` | stage 36's model |
| `bg\bg65535\stage_color.img` | character select lighting |
| `Bgm\bgm.txt` | the music table |

The first time the game reads its stage list, the mod writes the game's own `bg\BgList.txt` and
`bg\BgList_str.ini` into `MBTL-IM\Mods\bg` as plain text, so there is a readable copy to start from. From then on
those copies are the stage list. After a game update, delete them and start the game once to get fresh ones.

## Rules

- Files are plain, unencrypted files, the same thing an unpacker writes out.
- New files work, not only replacements, so new stages can be added.
- Two mods that change the same file, such as `bg\BgList.txt`, have to be merged by hand.

## The log

`MBTL-IM\Logs` holds one log per launch. It shows whether the loader found the game's file reader, every
file it served, and every file the game looked for in a modded folder but did not find. If a game update
moves things the loader turns itself off, the game runs unmodded, and the log says why.