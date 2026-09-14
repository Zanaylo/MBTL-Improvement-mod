# Replacing game files

Put a file under `MBTL-IM\Mods` at the same path it has inside the game's archives.

| File in Mods | What it does |
|---|---|
| `bg\BgList.txt` | the stage table |
| `bg\BgList_str.ini` | stage names |
| `bg\bg036\bg.fbx.bin` | stage 36's model |
| `bg\bg65535\stage_color.img` | character select lighting |
| `Bgm\bgm.txt` | the music table |

The first time the game reads its stage list, the mod saves the game's own `bg\BgList.txt` and
`bg\BgList_str.ini` to `MBTL-IM\Mods\bg` as plain text. From then on those copies are the stage list. After a game
update, delete them and start the game once to get fresh copies.

## Stage values without editing BgList.txt

A `stage.txt` in a stage folder holds the values that would otherwise go in its `Bg_` block of `BgList.txt`:

```
Name = "My stage"
FOV = 43.0,
ShadowLightType = 0,
ShadowLightStatus =
[
	{ Type=1, Position=0.0, PowerValue=7.0, Color=[0.0,0.0,0.0,0.8] },
],
StageSelTex = 45,
```

A whole `Bg_090 = { ... }` block copied from `BgList.txt` works too.

* In a stage folder you import (Stages > Add stages > A stage folder of your own), every value is used.
* In `Mods\bg\bgNNN\stage.txt` for one of the game's own stage numbers, it changes that stage.

`StageW` is the one value that is never used. Every stage keeps the game's wall distance (`StageW = 4096`), so both
players always have the same walls online.

## Replacing one of the game's stages

Stages > Add stages > In place of one of the game's stages copies a stage folder over a stage the game already has.
The stage keeps its number, card and music, so it needs no free number. An opponent who does not have it plays the
game's stage instead of crashing. The walls and the selection flags stay the game's, so the match stays in sync.

Restore, under Installed stages, brings the game's stage back.

## Stage select cards

Put a `thumbnail.png` or `thumbnail.dds` next to `bg.fbx.bin`. The mod paints it into the stage select and points
the stage at it. Any size works: it is scaled to fill the card and cut to the card's shape. An image of exactly
144x336 is used as it is, transparency included. DDS files can be uncompressed, DXT1 or DXT5.

A stage with a thumbnail always uses it as its card. A stage without one uses the card picked in the Stages window,
then its `StageSelTex`, then stage 1's card. Changes show after a restart.

By hand: `StageSelTex` 0 to 20 are on `grpdat\CSel\stage_thumb00.dds`, 21 and up on `stage_thumb01.dds`, seven
cards per row of 144x336 pixels. The game's own art stops at card 30. To add rows, make
`MBTL-IM\Mods\grpdat\CSel\stage_thumb01.dds` taller with a power of two height: 1024x2048 reaches card 62.

## Removing a stage

Remove, under Installed stages, deletes the stage's folder and takes it off the list right away. The game still
lists it until you restart, so do not pick it before then.

## Rules

* Files are plain and unencrypted, the same files an unpacker writes out.
* New files work too, not only replacements, so new stages can be added.
* Two mods that change the same file, such as `bg\BgList.txt`, have to be merged by hand.

## The log

`MBTL-IM\Logs` holds one log per launch. It shows whether the loader found the game's file reader, every file it
served, and every file the game looked for in a modded folder but did not find. If a game update breaks the loader,
it turns itself off, the game runs unmodded, and the log says why.
