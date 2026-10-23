/*----------------------
 | fb.c
 | Description: Ring Ring Land's 32x32 frame buffer (one word per row) and the 128-byte state block copied in from the app at start.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: fb
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | reverse8
 | Description: Reverses a byte's bits: the PocketStation LCD's leftmost pixel is bit 0, the VMU's bit 7.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: b -- byte
 | Returns: the reversed byte
 ----------------------*/
static uint8_t reverse8(uint8_t b) {
    uint8_t out = 0;
    for (uint8_t k = 0; k < 8u; k++) {
        out = (uint8_t)((out << 1) | ((b >> k) & 1u));
    }
    return out;
}

/*----------------------
 | ROM_STATE_INIT
 | Description: The app's initial copy of the 128-byte state block at 0x4C4.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_STATE_INIT 0x02000300u

/*----------------------
 | game_fb_put_rows
 | Description: Copies count row words from src into the frame buffer from row on, inverted when asked.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb
 | Params: row -- first row; count -- rows; src -- source address; invert -- nonzero to invert
 | Returns: N/A
 ----------------------*/
void game_fb_put_rows(uint8_t row, uint8_t count, uint32_t src, uint8_t invert) {
    do {
        uint32_t w = rom32(src);
        set_fb(row, invert ? ~w : w);
        src += 4u;
        row++;
    } while (--count);
}

/*----------------------
 | game_fb_present
 | Description: Copies the frame buffer to the LCD.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_fb_present(void) {
    uint8_t *lcd = vmu_fb();
    for (uint8_t row = 0; row < 32u; row++) {
        uint32_t w = ram_fb(row);
        uint8_t *p = lcd + row * 6u;
        p[0] = 0;
        for (uint8_t k = 0; k < 4u; k++) {
            p[1u + k] = reverse8((uint8_t)(w >> (8u * k)));
        }
        p[5] = 0;
    }
}

/*----------------------
 | game_fb_clear
 | Description: Clears the frame buffer.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_fb_clear(void) {
    for (uint8_t row = 0; row < 32u; row++) {
        set_fb(row, 0);
    }
}

/*----------------------
 | game_state_reset
 | Description: Copies the app's initial 128-byte state block into RAM at 0x4C4.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: the block 0x4C4-0x543
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_state_reset(void) {
    for (uint8_t i = 0; i < 0x80u; i += 4u) {
        vmu_w32((uint16_t)(STATE_BLOCK + i), rom32(ROM_STATE_INIT + i));
    }
}
