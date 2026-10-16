# PS1 reference screenshots

The user's DuckStation captures of the US new-game path, in play order; both ports diff against these and nobody should ask for them again.

| File | Screen |
|---|---|
| 01-title.png | TITLE menu |
| 02-gname-select-character.png | GNAME character select |
| 03-gname-select-weapon.png | GNAME weapon select |
| 04-gname-name-entry.png | GNAME name entry |
| 05-wsel-room.png | WSEL room with the wall map |
| 06-wsel-prompt.png | WSEL "Select play area" prompt |
| 07-wsel-cursor.png | WSEL area cursor |
| 08-wsel-cursor-confirmed.png | WSEL area cursor, chosen area lit |
| 09-wmap-loading.png | WMAP fade-in globe |
| 10-wmap-mailbox.png | WMAP board, mailbox label |
| 11-wmap-mailbox-carried.png | WMAP board, mailbox carried |
| 12-wmap-place-mailbox-prompt.png | WMAP "Place the Mailbox" prompt |
| 13-wmap-land-flash.png | WMAP land placement flash |
| 14-wmap-home-rising.png | WMAP Home rising, dark board |
| 15-wmap-home-dark.png | WMAP Home placed, dark board |
| 16-wmap-board-home.png | WMAP board with Home |
| 17-field-battle.png | FIELD battle: hero and ally against two bees in a rocky canyon land, both HP bars 100% (822x570 window capture) |

These are 2424x1080 screen captures with black pillarboxes, not native 320x240 frames, so a diff must crop the 4:3 picture and scale it down first. `saturn/reference/ps1-*.png` holds an older subset of the same screens.

`crops.json` holds each screenshot's detected 4:3 crop box; regenerate it with `python dreamcast/tools/frame_diff.py detect` (no screenshot needed its fallback crop). 04 leaves its own left columns black, so it takes 03's box (`CROP_SIBLINGS`).

Flycast grabs are compared after `frame_diff.load_capture` keeps rows 16..463 (display lines 8..231 at 2x), matching DuckStation's default NTSC overscan crop these screenshots were taken with.
