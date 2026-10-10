/*----------------------
 | field_text_words.h
 | Description: Converts FIELD's PS1 packed GPU packet words to the words that hold the same bytes in memory order, so text-window packets built from packed words read right on big-endian builds.
 | Author: suinevere
 | Dependencies: common.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef FIELD_TEXT_WORDS_H
#define FIELD_TEXT_WORDS_H

#include "common.h"

/*----------------------
 | FIELD_TEXT_COLOR_WORD / FIELD_TEXT_PAIR_WORD / FIELD_TEXT_UV_WORD
 | Description: A PS1 packed word as memory holds it: r, g, b and code bytes; two s16 (xy or wh); or u and v bytes then a u16 (clut or tpage).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: word -- PS1 packed value, low byte first
 | Returns: word to store
 ----------------------*/
#if defined(LOM_BIG_ENDIAN)
#define FIELD_TEXT_COLOR_WORD(word) __builtin_bswap32((u32)(word))
#define FIELD_TEXT_PAIR_WORD(word) (((u32)(word) << 16) | ((u32)(word) >> 16))
#define FIELD_TEXT_UV_WORD(word) ((((u32)(word) & 0xFF) << 24) | (((u32)(word) & 0xFF00) << 8) | ((u32)(word) >> 16))
#else
#define FIELD_TEXT_COLOR_WORD(word) (word)
#define FIELD_TEXT_PAIR_WORD(word) (word)
#define FIELD_TEXT_UV_WORD(word) (word)
#endif

#endif
