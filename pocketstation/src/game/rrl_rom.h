/*----------------------
 | rrl_rom.h
 | Description: Named addresses of Ring Ring Land's tables in the card file; addresses only, the bytes stay on the user's disc.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef RRL_ROM_H
#define RRL_ROM_H

/*----------------------
 | ROM_STRIPS
 | Description: Word table of symbol strip addresses; strip n runs from word n up to word n + 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_STRIPS 0x02009E00u

/*----------------------
 | ROM_NIBBLES_9DF0
 | Description: A nibble table in the app, eight per word, read by index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_NIBBLES_9DF0 0x02009DF0u

/*----------------------
 | ROM_ODDS_SEL
 | Description: Four words of byte weights summing to 100, chosen by sel_a | sel_b << 1; a pick is the index of the weight a roll below 100 lands in.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_ODDS_SEL 0x02009EDCu

/*----------------------
 | ROM_ODDS_SET
 | Description: Four words of byte weights summing to 100, chosen by odds_set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_ODDS_SET 0x02009EECu

/*----------------------
 | ROM_ODDS_NTH
 | Description: One word of byte weights summing to 100 that picks which item 02002D8A looks for.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_ODDS_NTH 0x02009EF8u

/*----------------------
 | ROM_EVENT_ODDS
 | Description: Word table, by scene_id - 7, of halfword lists whose low byte is a weight and high byte the scene it picks.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_EVENT_ODDS 0x02009F0Cu

/*----------------------
 | ROM_ANIM_DELAY
 | Description: A byte per animation id; a new animation's timer starts at a random value from 1 to it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_ANIM_DELAY 0x0200A7B0u

/*----------------------
 | ROM_SYMBOLS_10X9
 | Description: Packed 10x9 bitmaps of the reel symbols, 12 bytes each, drawn in the inverted reel window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_SYMBOLS_10X9 0x02006520u

/*----------------------
 | ROM_SYMBOLS_8X11
 | Description: Packed 8x11 bitmaps of the reel symbols, 12 bytes each, drawn on the three reels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_SYMBOLS_8X11 0x02007DBCu

/*----------------------
 | ROM_MARK_OFFSETS
 | Description: Signed (x, y) byte pairs placing an icon's marks: one pair for one mark, then two for two, then three for three.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_MARK_OFFSETS 0x0200A7A4u

/*----------------------
 | ROM_WORDS_8094
 | Description: A table of 255 item halfwords in the app: low byte the kind, high byte the odds pick, top bit a flag game_rom_word_8094 strips.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_WORDS_8094 0x02008094u

#endif
