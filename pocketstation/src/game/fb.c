/*----------------------
 | fb.c
 | Description: Ring Ring Land's 32x32 frame buffer (one word per row) and the 128-byte state block copied in from the app at start.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h
 | Globals: fb
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"

/*----------------------
 | LCD_VRAM
 | Description: The LCD's 32 row words.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LCD_VRAM 0x0D000100u

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
        uint32_t w = psk_ldr(src);
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
    for (uint8_t row = 0; row < 32u; row++) {
        psk_w32(LCD_VRAM + row * 4u, ram_fb(row));
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
        psk_w32(STATE_BLOCK + i, psk_r32(ROM_STATE_INIT + i));
    }
}
